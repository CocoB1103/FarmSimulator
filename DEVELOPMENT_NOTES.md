# Notes de développement — Farm Simulator

Ce document liste les problèmes rencontrés pendant le développement de ce projet
(Unreal Engine 5.7, 100% C++, pas de Blueprints) et leurs solutions, pour éviter
de perdre du temps à les redécouvrir lors de futurs développements.

---

## 1. Prérequis obligatoires

- **Unreal Engine 5.7** installé (`C:\Program Files\Epic Games\UE_5.7`).
- **Visual Studio 2022 Build Tools** avec le workload *Desktop development with C++*
  (MSVC v143, build tools >= 14.38, idéalement 14.44). Sans ça, `UnrealBuildTool`
  échoue avec : `No valid Visual C++ toolchain was found`.
  Installable en une commande (nécessite une élévation UAC) :
  ```powershell
  winget install --id Microsoft.VisualStudio.2022.BuildTools -e --silent --override "--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --includeRecommended"
  ```

---

## 2. Le projet NE DOIT PAS être dans un dépôt Git contenant des noms de
   fichiers avec des caractères spéciaux/accents/emoji

**Symptôme** : `UnrealBuildTool` plante avec une exception .NET (pas un message
d'erreur propre) pendant la phase *"Creating makefile"* :
```
System.ArgumentException: Path fragment '"FarmSimulator/\342\234\205_COMPLET.txt"'
contains invalid directory separators.
   at UnrealBuildTool.GitSourceFileWorkingSet.AddPath(String Path)
```

**Cause** : UBT utilise `git status` pour la fonctionnalité *adaptive non-unity
build*. Si le dépôt Git (même un dossier parent) contient des fichiers avec des
caractères Unicode (accents, emoji...), `git status` les affiche sous forme
échappée (octal) à cause de `core.quotepath`, et le parseur de chemins de UBT
plante dessus — même si ces fichiers n'ont rien à voir avec le projet Unreal.

**Solution appliquée** : le projet `FarmSimulator` est volontairement situé
**en dehors** de `C:\Users\Corentin\Desktop\JeuDev` (qui est un dépôt Git
contenant de tels fichiers). Le projet a son propre dépôt Git indépendant.

**Solutions alternatives** si on doit un jour remettre le projet dans un repo
"pollué" : renommer les fichiers problématiques, ou désactiver l'adaptive unity
build via `BuildConfiguration.xml` (`<bUseAdaptiveUnityBuild>false</bUseAdaptiveUnityBuild>`),
ou `git config core.quotepath false`.

---

## 3. Choix d'architecture : 100% C++, pas de Blueprints

Le jeu devait être généré de façon 100% autonome, sans jamais ouvrir l'éditeur
pour câbler des graphes à la main. Deux options existaient :

- **Scripter les Blueprints via Python** (`unreal.BlueprintEditorLibrary`) :
  l'API publique ne permet quasiment pas de créer des graphes d'event/logique
  de façon fiable (c'est fait pour créer des assets simples, pas du gameplay).
- **Tout coder en C++** : compilable et testable en ligne de commande, sans
  dépendre de l'UI de l'éditeur. **C'est l'approche retenue.**

Conséquence : le niveau (`Map_Farm`) est quasiment **vide dans l'éditeur**
(0 acteurs). Tout le monde (sol, parcelles, clôtures, lumières, bâtiment,
musique) est généré par code dans `AFarmGameMode::InitGame()`. C'est voulu,
pas un bug — si quelqu'un ouvre la map et ne voit rien, c'est normal tant
qu'on n'a pas appuyé sur Play.

Seules deux choses ont dû être créées via l'éditeur/Python (impossible à écrire
à la main, ce sont des formats binaires) :
- Le niveau `Map_Farm.umap` (vide).
- Le matériau `M_Base_Color` (un matériau avec un paramètre vecteur "Color" et
  un scalaire "Roughness", utilisé pour tinter dynamiquement les meshes de base).

---

## 4. Importer des assets (FBX / sons) via Python plante en mode headless pur

**Symptôme** : lancer `UnrealEditor-Cmd.exe <projet> -run=pythonscript
-script=import.py -unattended` plante avec :
```
Assertion failed: CurrentApplication.IsValid()
[...Slate\Public\Framework\Application\SlateApplication.h] [Line: 321]
```
au moment d'importer un FBX (`AssetTools.import_asset_tasks`), alors que la
création d'assets simples (matériaux, niveaux) fonctionne très bien dans ce
mode.

**Cause** : le système d'import **Interchange** (utilisé pour les FBX depuis
UE5.x) essaie d'utiliser des widgets Slate (barres de progression, dialogues
de pipeline). Le mode `-run=<commandlet>` (même avec `-unattended`) ne crée
jamais de `FSlateApplication`, donc ça plante — peu importe qu'on utilise
`UnrealEditor.exe` ou `UnrealEditor-Cmd.exe`.

**Solution** : ne PAS utiliser `-run=pythonscript`. Lancer l'éditeur
normalement (sans `-run=`) avec `-ExecCmds="py <chemin_du_script>"` : dans ce
mode, l'éditeur démarre complètement (Slate disponible) puis exécute le script
une fois prêt. Terminer le script par `unreal.SystemLibrary.quit_editor()`
pour que l'éditeur se referme tout seul (sinon il reste ouvert).
```powershell
UnrealEditor.exe "Projet.uproject" -ExecCmds="py C:/chemin/script.py" -stdout -nosplash
```

⚠️ Si `quit_editor()` est appelé **trop tôt** dans le cycle de vie de
l'éditeur (juste après un `-ExecCmds` qui se termine très vite), ça peut
corrompre le layout de l'éditeur sauvegardé (voir section 6).

---

## 5. Calibrer l'échelle de vrais assets 3D (Kenney, FBX) sans les voir

Pour connaître la taille réelle (en cm) d'un mesh importé sans ouvrir l'éditeur
visuellement, on peut spawner temporairement un `StaticMeshActor` via Python et
lire ses bounds :
```python
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0,0,0))
actor.static_mesh_component.set_static_mesh(mesh)
origin, extent = actor.get_actor_bounds(only_colliding_components=False)
# taille réelle = extent * 2
```
(Ne pas utiliser `mesh.get_editor_property("extended_bounds")` : cette
propriété n'existe pas côté Python sur `StaticMesh`.)

---

## 6. BUG MAJEUR UMG : construire l'arbre de widgets dans `NativeConstruct()`
   ne fonctionne PAS pour un widget 100% C++

**Symptôme** : le HUD ne s'affiche jamais à l'écran (aucune erreur, aucun
warning). `CreateWidget` renvoie un objet valide, `AddToViewport()` fonctionne
(`IsInViewport() == true`), `NativeConstruct()` s'exécute sans erreur et peuple
bien `WidgetTree`... mais rien n'est visible, pas même un `GEngine->
AddOnScreenDebugMessage` placé juste avant (celui-là s'affichait bien, ce qui
a permis d'isoler le problème : le pipeline de rendu overlay fonctionnait,
seul le widget UMG posait problème).

**Cause réelle** : `RebuildWidget()` (qui transforme `WidgetTree` en une
vraie hiérarchie Slate) est appelé **avant** `NativeConstruct()` dans le cycle
de vie d'un `UUserWidget` (il est déclenché par `TakeWidget()`, utilisé en
interne par `AddToViewport()`). Si on construit l'arbre de widgets (`WidgetTree
->ConstructWidget<...>`) à l'intérieur de `NativeConstruct()`, c'est donc
**trop tard** : la hiérarchie Slate a déjà été construite à partir d'un
`WidgetTree` vide, et les modifications faites ensuite n'ont plus d'effet sur
le rendu déjà figé.

**Solution** : construire tout l'arbre de widgets dans
`virtual void NativeOnInitialized() override` à la place (appelé par
`Initialize()`, bien avant le premier `RebuildWidget()`). `NativeConstruct()`
reste réservé à la logique "par instance" qui peut arriver après coup (bind de
délégués, timers...). Voir `FarmHUDWidget.cpp`.

```cpp
// MAUVAIS : trop tard, le widget ne s'affichera jamais correctement
virtual void NativeConstruct() override { /* construire WidgetTree ici */ }

// BON
virtual void NativeOnInitialized() override { /* construire WidgetTree ici */ }
```

### Comment on l'a détecté sans voir l'écran
`UnrealEditor-Cmd.exe -game` tourne sans fenêtre visible pour l'agent. Les
captures via la commande console `Shot` ne contiennent PAS la couche UMG/Slate
finale (elle capture une passe de rendu antérieure à la composition de l'UI).
Pour diagnostiquer un problème d'affichage UI, il faut faire une **vraie**
capture Windows de la fenêtre (GDI), pas la commande `Shot` d'Unreal :
```powershell
# PrintWindow capture le contenu réel de la fenêtre (y compris l'UI composée),
# contrairement à la commande console "Shot". Il faut activer la prise en
# compte du DPI (SetProcessDPIAware) sinon GetClientRect/PrintWindow
# ne capture qu'une partie de la fenêtre réelle.
[Win32]::SetProcessDPIAware()
[Win32]::PrintWindow($hwnd, $hdc, 2)  # 2 = PW_RENDERFULLCONTENT
```

---

## 7. Shadowing de variables avec les classes de base Unreal

`AGameModeBase` possède déjà un membre protégé nommé `GameState`, et
`UUserWidget`/`UWidget` possèdent un membre nommé `Slot`. Nommer une variable
locale `GameState` ou `Slot` dans une fonction dérivée compile en erreur
(`C4458`, traité comme erreur car `-WarningsAsErrors` est actif pour UHT) :
```
error C4458: la déclaration de 'GameState' masque le membre de classe
```
→ Toujours préfixer/renommer les variables locales qui risquent d'entrer en
collision (`FarmGameState`, `CanvasSlot`, etc.).

---

## 8. Licences des assets utilisés

- **Modèles 3D** : packs *Nature Kit* et *Food Kit* de [Kenney](https://kenney.nl)
  — licence **CC0** (domaine public, aucune attribution requise).
  Fichiers sources dans `PyAutomation/` (scripts d'import) ; les `.uasset`
  compilés sont dans `Content/Assets/`.
- **Musique** : *"Menu Title 1 (relaxed)"* du pack *12 Music Loops* par
  SubspaceAudio, hébergé sur OpenGameArt.org — licence **CC0**.

Aucune attribution n'est légalement requise, mais il est de bon ton de garder
une trace des sources (fait ici) en cas de réutilisation commerciale.

---

## 9. Rééquilibrage économique

Les prix d'origine du design doc (`Prix` d'achat == `Valeur` de vente, marge
nette de 0$) ont été volontairement modifiés pour qu'il y ait un vrai profit :
voir `FarmTypes.cpp` (`BuildCropDatabase`) pour les valeurs actuelles.

---

## 10. Pièges liés à l'automatisation de l'éditeur (pour de futurs scripts)

- Fermer l'éditeur de force (`Stop-Process`) ou appeler `quit_editor()` très
  tôt après un lancement peut corrompre le layout UI sauvegardé, causant un
  `Fatal error: Object is not packaged: ModeManagerInteractiveToolsContext`
  au lancement suivant. **Solution** : supprimer
  `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` (projet) et
  `%LOCALAPPDATA%/UnrealEngine/5.7/Saved/Config/WindowsEditor/EditorLayout.ini`
  (global) pour forcer la régénération d'un layout propre.
- Toujours préférer `-ExecCmds="py script.py"` avec l'éditeur complet plutôt
  que `-run=pythonscript` dès qu'un script touche à l'import d'assets ou à
  autre chose que des opérations purement "data" (voir section 4).

---

## 11. Pistes d'amélioration futures

- Ajouter des sons d'interaction (récolte, vente, plantation).
- Ajouter une interface de pause / menu principal.
- Restreindre certaines cultures à certaines saisons pour plus de stratégie.
- Remplacer le personnage joueur (actuellement un cylindre coloré) par un
  vrai modèle de personnage.
- Ajouter plusieurs emplacements de sauvegarde (slots).
- Passer les décorations (arbres, clôtures) en *Instanced Static Mesh* si leur
  nombre augmente, pour les performances.

GraphiqueBourse
Application de bureau Windows native (C++ / Win32) de visualisation de graphiques boursiers en temps quasi réel,
développée en collaboration avec Claude Sonnet (Anthropic).

Ce projet date de 2007. Son utilité : les données de l'intraday n'ont que quelques secondes de retard.


⚠️ Avertissement — Les graphiques affichés proviennent d'ADVFN. Leurs CGU interdisent l'accès programmatique sans accord écrit préalable. Cette application est destinée à un usage personnel uniquement.


Fonctionnalités

Affichage du graphique intraday avec rafraîchissement automatique toutes les 10 secondes
Affichage du graphique durée (1M, 2M, 3M, 6M, 1Y, 2Y, 3Y, 5Y)
Choix du type de graphique (ligne, area, chandelier, barre)
Calcul du coût d'acquisition PEA (courtage, TTF, seuil de rentabilité)
Support de 32 instances simultanées et indépendantes
Persistance automatique de la position et de l'indice sélectionné par instance
DPI-aware (support des écrans haute résolution)
Absente de la barre des tâches Windows


Indices et titres disponibles
Indices
LibelléMarchéCAC 40FranceSBF 120FranceDAX 30AllemagneFTSE 100Royaume-UniS&P 500États-UnisDow JonesÉtats-UnisNASDAQÉtats-Unis
Forex
PaireEUR/USDEUR/JPYEUR/GBPEUR/CHF
Actions Paris
Liste complète des actions de la Bourse de Paris — source : boursedeparis.fr → Marchés au comptant → Actions → liste complète.
La liste couvre l'ensemble des valeurs du marché au comptant parisien, de 2CRSI à ZCI Limited.
Mise à jour de la liste
Dans Excel :

Colonne A : Nom — colonne B : Symbole ADVFN
Colonne E : ="L"""& A1 &"""," → génère le libellé
Colonne F : ="L"""& B1 &"""," → génère le mnémonique
Dans LABELS, échapper les apostrophes ' → \'
Copier les colonnes E et F dans LABELS et MNEMOS
Supprimer la virgule finale de chaque colonne


Utilisation
ActionEffetDouble clic gauche sur le graphique intradayAfficher / masquer le graphique duréeClic droit sur le graphique intradaySupprimer définitivement cette instanceClic gauche sur le graphique duréeAfficher / masquer les options et les calculsClic gauche sur les résultatsMasquer les résultatsBouton ? dans la barre de titreAfficher l'aide

Diagrammes
États de la fenêtre
```mermaid
mermaidstateDiagram-v2
    [*] --> STATE_COLLAPSED : démarrage
    STATE_COLLAPSED --> STATE_MEDIUM : double clic intraday
    STATE_MEDIUM --> STATE_COLLAPSED : double clic intraday
    STATE_MEDIUM --> STATE_EXPANDED : bouton Calcul
    STATE_EXPANDED --> STATE_MEDIUM : clic résultats
    STATE_MEDIUM --> STATE_COLLAPSED : clic résultats

    state STATE_MEDIUM {
        [*] --> GrapheDuree : g_GraphDureeOuOptions = TRUE
        GrapheDuree --> Effecteurs : clic graphe durée / btnToggle
        Effecteurs --> GrapheDuree : clic graphe durée / btnToggle
    }
```
Séquence de démarrage
```mermaid
mermaidsequenceDiagram
    participant Main as wWinMain
    participant Reg as Registre
    participant Mutex as Mutex système
    participant Win as WindowProc
    participant ADVFN

    Main->>Mutex: FindFreeInstanceSlot()
    Mutex-->>Main: instanceId = N
    Main->>Mutex: CreateInstanceMutex(N)
    Main->>Reg: LoadInstanceConfig(N)
    Reg-->>Main: index, x, y
    Main->>Win: CreateWindowEx → WM_CREATE
    Win->>Win: CreateControls()
    Win->>Win: UpdateLayout(refreshResources=TRUE)
    Win->>ADVFN: DownloadAndDisplayImage(mnemo, FALSE)
    ADVFN-->>Win: image intraday (~5 Ko)
    Win->>Win: SetTimer(TIMER_GRAPH, 10s)
    alt Instance principale (slot 0)
        Main->>Reg: LoadTotalInstances()
        loop i = 1..total
            Main->>Main: LaunchInstance(i) si mort
        end
    end
```
Cycle de rafraîchissement intraday
```mermaid
mermaidsequenceDiagram
    participant Timer as WM_TIMER (10s)
    participant DL as DownloadAndDisplayImage
    participant URLMon as URLOpenBlockingStreamW
    participant GDI as GDI+
    participant UI as STATIC g_hIntraday

    Timer->>DL: mnemo, forDuree=FALSE
    DL->>URLMon: GET /p.php?...&p=0&t=23&dm=0
    URLMon-->>DL: IStream (~5 Ko)
    DL->>GDI: Bitmap::FromStream()
    GDI-->>DL: Bitmap*
    DL->>GDI: GetHBITMAP(bgColor)
    GDI-->>DL: HBITMAP hRaw
    DL->>DL: StretchBitmap(hRaw, targetW, targetH)
    DL->>UI: STM_SETIMAGE
    UI-->>UI: InvalidateRect / UpdateWindow
```
Séquence de téléchargement durée
```mermaid
mermaidsequenceDiagram
    participant User as Utilisateur
    participant Win as WindowProc
    participant DL as DownloadAndDisplayImage
    participant URLMon as URLOpenBlockingStreamW
    participant UI as STATIC g_hDuree

    User->>Win: double clic intraday → STN_DBLCLK
    Win->>Win: ToggleGraphique() → STATE_MEDIUM
    Win->>DL: mnemo, forDuree=TRUE
    DL->>URLMon: GET /p.php?...&p=P&t=49&dm=DM&vol=0
    URLMon-->>DL: IStream (~5 Ko)
    DL->>UI: STM_SETIMAGE
```
Suppression d'une instance
```mermaid
mermaidsequenceDiagram
    participant User as Utilisateur
    participant Sub as IntraSubclassProc
    participant Menu as ContextMenuIntraday
    participant Reg as Registre
    participant Mutex as Mutex système
    participant Win as WM_DESTROY

    User->>Sub: clic droit sur intraday
    Sub->>Menu: WM_RBUTTONUP
    Menu->>Menu: TrackPopupMenu → "Supprimer"
    Menu->>Menu: g_deletingInstance = TRUE
    Menu->>Reg: DeleteInstanceConfig(N)
    Menu->>Mutex: ReleaseMutex + CloseHandle
    Note over Mutex: slot N détecté libre par les autres instances
    Menu->>Win: DestroyWindow → WM_DESTROY
    Win->>Win: PostQuitMessage(0)
```
DoLayout — double mode MEASURE / PLACE
```mermaid
mermaidflowchart TD
    A[UpdateLayout appelé] --> B{refreshResources ?}
    B -- oui --> C[Recréer police + invalider bitmaps]
    B -- non --> D
    C --> D[DoLayout LAYOUT_MEASURE]
    D --> E[Calcul posY sans toucher les contrôles]
    E --> F[ClientToWindow → wWidth, wHeight]
    F --> G[SetWindowPos fenêtre]
    G --> H[DoLayout LAYOUT_PLACE]
    H --> I[SetWindowPos chaque contrôle]
    I --> J[ShowWindow sections visibles]
    J --> K{refreshResources ?}
    K -- oui --> L[DownloadAndDisplayImage intraday]
    L --> M{STATE_MEDIUM && GraphDuree ?}
    M -- oui --> N[DownloadAndDisplayImage durée]
    M -- non --> O[InvalidateRect / UpdateWindow]
    N --> O
    K -- non --> O
```
Gestion des instances — coordination par mutex
```mermaid
mermaidflowchart LR
    subgraph Processus A - slot 0
        MA[Mutex Instance_0\novert]
    end
    subgraph Processus B - slot 1
        MB[Mutex Instance_1\novert]
    end
    subgraph Processus C - slot 2
        MC[Mutex Instance_2\novert]
    end
    subgraph Registre HKCU
        R0[Instance0_Index / X / Y]
        R1[Instance1_Index / X / Y]
        R2[Instance2_Index / X / Y]
    end
    subgraph Mutex registre
        MR[GraphiqueBourse_Registry_Mutex\naccès exclusif]
    end

    MA -- protège --> R0
    MB -- protège --> R1
    MC -- protège --> R2
    MR -- sérialise --> R0
    MR -- sérialise --> R1
    MR -- sérialise --> R2
```
Multi-instances
Chaque instance est indépendante et peut afficher un indice ou une action différente. Les instances sont identifiées par un numéro de slot (0 à 31) et leur configuration (indice, position) est sauvegardée dans le registre Windows sous :
HKEY_CURRENT_USER\Software\GraphiqueBourse\Instances
Au démarrage, l'instance principale (slot 0) restaure automatiquement toutes les instances précédemment ouvertes.

Calcul PEA
La section calcul permet d'estimer le coût réel d'un ordre d'achat en PEA :

Courtage : minimum 2,00 € — 0,45% au-delà de 500 € — plafonné à 0,50%
TTF : taxe sur transactions financières (0,4%), optionnelle
Taux d'imposition : 18,6% pour un PEA de plus de 5 ans, 30% (PFU) avant 5 ans
Seuil de rentabilité : tenant compte du courtage aller/retour et de la fiscalité PEA


Architecture technique

Win32 pur — aucune dépendance framework (pas de MFC, Qt, Electron)
GDI+ — décodage et redimensionnement des images
URLmon — téléchargement synchrone des graphiques (URLOpenBlockingStreamW)
Mutex nommés — coordination des instances vivantes
Registre Windows — persistance de la configuration
DoLayout MEASURE/PLACE — source de vérité unique pour le layout, garantissant la cohérence à tout DPI
Fenêtre propriétaire cachée (GraphiqueBoursierOwner) — masquage de la barre des tâches sans perdre l'icône de titre
CS_DBLCLKS sur la classe de fenêtre — nécessaire pour que le contrôle STATIC reçoive STN_DBLCLK

Consommation réseau maximale (32 instances)
32 instances × 2 images × 5 Ko × 6 / min ≈ 1,9 Mo/min = 0,25 Mbit/s

Compilation
Prérequis

Visual Studio 2019 ou supérieur (MSVC)
SDK Windows 10 ou supérieur

-------------------------------------------------------------------------------------------------------------------------------
  actualisation de la liste des titres. https://www.boursedeparis.fr/cours/actions-paris
  aller sur la brique Marchés au comptant puis,
  sur Action la liste complete est dans le lien en haut sur la droite.

    Dans excel :
      La colonne A1 on doit échaper les apostraphes "\'"
	  		Name   : A1 : 2CRSI --> E1 : ="L"""& A1 &""","
	  		Symbol : B1 : AL2SI --> F1 : ="L"""& B1 &""","

	  	Copiez les colonnes E, F.

  Il fait supprimer la virgule finale de chaques colonnes.

ce projet date de 2007. Son utilité, les données de l'intraday n'ont que quelques seconde de retard.

-------------------------------------------------------------------------------------------------------------------------------

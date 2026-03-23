
/* <!---------------------------------------------------------------------------------------------------------------------------------------
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

---------------------------------------------------------------------------------------------------------------------------------------->
*/

# include <windows.h>
# include <commctrl.h>
# include <urlmon.h>
# include <gdiplus.h>
# include <shellapi.h>
# include <shobjidl.h>
# include <richedit.h>
# include <algorithm>
# include <vector>
# include <string>
# include <cstdio>

# include "graphbourse.h"

# pragma comment (lib, "comctl32.lib")
# pragma comment (lib, "urlmon.lib")
# pragma comment (lib, "gdiplus.lib")
# pragma comment (lib, "User32.lib")
# pragma comment (lib, "Gdi32.lib")
# pragma comment (lib, "Kernel32.lib")
# pragma comment (lib, "Shell32.lib")
# pragma comment (lib, "Advapi32.lib")
# pragma comment (lib, "Ole32.lib")

using namespace Gdiplus ;
using namespace std ;

// ==================================== DEBUG ====================================
static const BOOL Debug = FALSE ; // Mode debug : affiche une console et des traces
static SYSTEMTIME st ;    // Mode debug : temp systeme en ms.
static VOID LogTrace (const char * func) {
  GetLocalTime (& st) ;
  wprintf (L"[%S] %02d:%02d:%02d.%03d\n", func, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds) ;
}
// ===============================================================================

// CONSTANTES

static const INT  IMG_W          = 165 ;   // Largeur des images en pixels logiques (96 DPI)
static const INT  IMG_H_INTRA    = 116 ;   // Hauteur de l'image intraday
static const INT  IMG_H_DUREE    = 101 ;   // Hauteur de l'image graphe durée
static const INT  LIST_H         = 20 ;    // Hauteur de la combobox principale
static const INT  TIMER_GRAPH    = 1 ;     // Identifiant du timer de rafraîchissement
static const INT  TIMER_INTERVAL = 10000 ; // Intervalle de rafraîchissement en ms (10 secondes)
static const INT  MAX_INSTANCES  = 32 ;    // Nombre maximum d'instances simultanées supportées

// Clé registre de sauvegarde des instances
const wstring REG_KEY = L"Software\\GraphiqueBourse\\Instances" ;

// Noms des mutex globaux :
//   MUTEX_REGISTRY : accès exclusif au registre partagé entre instances
static const wstring MUTEX_REGISTRY = L"GraphiqueBourse_Registry_Mutex" ;
//   MUTEX_INSTANCE : préfixe du mutex de vie d'une instance (+ numéro)
static const wstring MUTEX_INSTANCE = L"GraphiqueBourse_Instance_" ;

// ÉNUMÉRATIONS

enum HeightState {
  // États d'expansion de la fenêtre :
  //   STATE_COLLAPSED : seule l'image intraday est visible
  //   STATE_MEDIUM    : intraday + graphe durée ou effecteurs
  //   STATE_EXPANDED  : medium + résultats de calcul
   STATE_COLLAPSED, STATE_MEDIUM, STATE_EXPANDED
  } ;

enum LayoutMode {
  // Mode d'exécution de DoLayout() :
  //   LAYOUT_MEASURE : calcule la hauteur client sans déplacer aucun contrôle
  //   LAYOUT_PLACE   : place tous les contrôles et retourne la hauteur client
  LAYOUT_MEASURE, LAYOUT_PLACE
} ;

enum {

  // Identifiants des contrôles enfants

  ID_LIST,                  // Combobox principale de sélection de l'indice
  ID_STATIC_IMAGE_INTRADAY, // Image intraday (clic gauche : expand/collapse, clic droit : menu)
  ID_STATIC_IMAGE_DUREE,    // Image graphe durée (cliquable pour basculer vers effecteurs)
  ID_COMBO_PERIODE,         // Combobox de sélection de la période du graphe durée
  ID_BUTTON_TOGGLE,         // Bouton bascule graphe durée / effecteurs
  ID_COMBO_TYPE,            // Combobox de sélection du type de graphe
  ID_CHECK_AVANT5ANS,       // Checkbox PEA de moins de 5 ans
  ID_CHECK_TTF,             // Checkbox titre soumis à TTF
  ID_EDIT_NOMBRE,           // Champ de saisie du nombre de titres
  ID_EDIT_VALEUR,           // Champ de saisie de la valeur unitaire
  ID_BUTTON_CALC            // Bouton de calcul
} ;

// DONNÉES : libellés et mnémoniques des indices

static const vector <wstring> LABELS = {
  L"♦",
  L".CAC40",
  L".SBF 120",
  L".Dax 30 (Allemagne)",
  L".FTSE 100 (Royaume Uni)",
  L".S&P 500 (U$A)",
  L".Dow Jones (U$A)",
  L".NASDAC (U$A)",
  L"♦",
  L".€uro/U$D",
  L".€uro/Yen",
  L".€uro/£ivre Sterling",
  L".€uro/Fr Suisse",
  L"♦",
  L"2CRSI",
  L"74SOFTWARE",
  L"AB SCIENCE",
  L"AB SCIENCE BSA",
  L"AB SCIENCE BSA 25",
  L"AB SCIENCE BSAAU25",
  L"ABC ARBITRAGE",
  L"ABEO",
  L"ABIONYX PHARMA",
  L"ABIVAX",
  L"ABL Diagnostics",
  L"ABO GROUP",
  L"ACANTHE DEV.",
  L"ACCOR",
  L"ACHETER-LOUER.FR",
  L"ACTEOS",
  L"ACTIA GROUP",
  L"ACTIVIUM GROUP",
  L"ADC SIIC",
  L"ADEUNIS",
  L"ADOCIA",
  L"ADOCIA BSA",
  L"ADOMOS",
  L"ADP",
  L"ADUX",
  L"ADVICENNE",
  L"ADVINI",
  L"AELIS FARMA",
  L"AERKOMM INC",
  L"AFYREN",
  L"AG3I",
  L"AGENCE AUTO",
  L"AGP MALAGA SOCIMI",
  L"AGRIPOWER",
  L"AGROGENERATION",
  L"AIR FRANCE -KLM",
  L"AIR LIQUIDE",
  L"AIRBUS",
  L"AIRWELL",
  L"AKWEL",
  L"ALAN ALLMAN",
  L"ALPES (COMPAGNIE)",
  L"ALSTOM",
  L"ALTAMIR",
  L"ALTAREA",
  L"ALTAREIT",
  L"ALTEN",
  L"ALTHEORA",
  L"ALVEEN",
  L"AMA CORPORATION",
  L"AMATHEON AGRI",
  L"AMOEBA",
  L"AMUNDI",
  L"ANDINO GLOBAL",
  L"ANTIN INFRA PARTN",
  L"APERAM",
  L"APODACA INVERSIONE",
  L"AQUILA",
  L"ARAMIS GROUP",
  L"ARCELORMITTAL SA",
  L"ARCHOS",
  L"ARCURE",
  L"ARDOIN ST AMAND A",
  L"ARDOIN ST AMAND B",
  L"AREF THALASSA",
  L"ARGAN",
  L"ARIMELIA ITG",
  L"ARKEMA",
  L"AROCA DEL PINAR",
  L"ARTEA",
  L"ARTMARKET COM",
  L"ARTOIS NOM.",
  L"ARVERNE GROUP",
  L"ARVERNE WARRANT",
  L"ASHLER ET MANSON",
  L"ASSYSTEM",
  L"ASTICKSO XXI",
  L"ATARI",
  L"ATEME",
  L"ATLAND",
  L"ATON",
  L"ATOS",
  L"AUBAY",
  L"AUDACIA",
  L"AUGROS COSMETIC",
  L"AUREA",
  L"AVENIR TELECOM",
  L"AXA",
  L"AXA NV26",
  L"AYVENS",
  L"AZ LEASING",
  L"AZOREAN TECH",
  L"BAIKOWSKI",
  L"BAINS MER MONACO",
  L"BALYO",
  L"BARBARA BUI",
  L"BARINGS CORE SPAIN",
  L"BASSAC",
  L"BASTIDE LE CONFORT",
  L"BEBO HEALTH",
  L"BENETEAU",
  L"BERNARD LOISEAU",
  L"BIC",
  L"BIGBEN INTERACTIVE",
  L"BILENDI",
  L"BIMMART INVESTMENT",
  L"BIO INOX",
  L"BIO-UV GROUP",
  L"BIOMERIEUX",
  L"BIOPHYTIS",
  L"BIOPHYTIS BSA",
  L"BIOPHYTIS BSA",
  L"BIOPHYTIS BSA31",
  L"BIOSENIC",
  L"BIOSYNEX",
  L"BLEECKER",
  L"BLUE SHARK POWER",
  L"BLUELINEA",
  L"BNP PARIBAS ACT.A",
  L"BOA CONCEPT",
  L"BODY ONE",
  L"BOIRON",
  L"BOLLORE",
  L"BONDUELLE",
  L"BONYF",
  L"BOOSTHEAT",
  L"BOURRELIER GROUP",
  L"BOURSE DIRECT",
  L"BOUYGUES",
  L"BOUYGUES NV26",
  L"BROADPEAK",
  L"BUREAU VERITAS",
  L"BURELLE",
  L"CA TOULOUSE 31 CCI",
  L"CABASSE",
  L"CAFOM",
  L"CALIBRE",
  L"CALIBRE BSA K2A",
  L"CALIBRE BSA K2B",
  L"CAMBODGE NOM.",
  L"CAPGEMINI",
  L"CAPITAL B",
  L"CAPITAL B BSA 26",
  L"CARBIOS",
  L"CARMILA",
  L"CARREFOUR",
  L"CARVOLIX",
  L"CASINO BSA1",
  L"CASINO BSA3",
  L"CASINO GUICHARD",
  L"CATANA GROUP",
  L"CATERING INTL SCES",
  L"CBI",
  L"CBI BSA A",
  L"CBI BSA B",
  L"CBO TERRITORIA",
  L"CEGEDIM",
  L"CELLECTIS",
  L"CELYAD ONCOLOGY",
  L"CFI",
  L"CFM INDOSUEZWEALTH",
  L"CH.FER DEPARTEMENT",
  L"CHARWOOD ENERGY",
  L"CHAUSSERIA",
  L"CHEOPS TECHNOLOGY",
  L"CHRISTIAN DIOR",
  L"CIBOX INTER A CTIV",
  L"CIE DU MONT BLANC",
  L"CIECHARGEURSINVEST",
  L"CLARANOVA",
  L"CLARIANE",
  L"CMG CLEANTECH",
  L"COFACE",
  L"COFIDUR",
  L"COGRA",
  L"COHERIS",
  L"COIL",
  L"COLIPAYS",
  L"COMPAGNIE ODET",
  L"CONDOR TECHNOLOG",
  L"CONSORT NT",
  L"CONSTRUCTEURS BOIS",
  L"CORE SPAIN HOLDCO",
  L"COREP LIGHTING",
  L"CORETECH 5",
  L"COTY",
  L"COURBET HERITAGE",
  L"COURTOIS",
  L"COVIVIO",
  L"COVIVIO HOTELS",
  L"CRCAM ALP.PROV.CCI",
  L"CRCAM ATL.VEND.CCI",
  L"CRCAM BRIE PIC2CCI",
  L"CRCAM ILLE-VIL.CCI",
  L"CRCAM LANGUED CCI",
  L"CRCAM LOIRE HTE L.",
  L"CRCAM MORBIHAN CCI",
  L"CRCAM NORD CCI",
  L"CRCAM NORM.SEINE",
  L"CRCAM PARIS ET IDF",
  L"CRCAM SUD R.A.CCI",
  L"CRCAM TOURAINE CCI",
  L"CREDIT AGRICOLE",
  L"CROSSJECT",
  L"CROSSJECT BS27",
  L"CROSSWOOD",
  L"D.L.S.I.",
  L"DAMARIS",
  L"DAMARTEX",
  L"DANONE",
  L"DASSAULT AVIATION",
  L"DASSAULT SYSTEMES",
  L"DBT",
  L"DBV TECHNOLOGIES",
  L"DEEZER",
  L"DEEZER WARRANTS",
  L"DEKUPLE",
  L"DELFINGEN",
  L"DELTA PLUS GROUP",
  L"DERICHEBOURG",
  L"DEVERNOIS",
  L"DIAGNOSTIC MEDICAL",
  L"DNXCORP",
  L"DOCK.PETR.AMBES AM",
  L"DOLFINES",
  L"DONTNOD",
  L"DRONE VOLT",
  L"DRONE VOLT BS28",
  L"DRONE VOLT BSA",
  L"DRONE VOLT BSA",
  L"DYNAFOND",
  L"E PANGO",
  L"EAGLEFOOTBALLGROUP",
  L"EASSON HOLDINGS",
  L"EAUX DE ROYAN",
  L"EAVS",
  L"ECOMIAM",
  L"ECOSLOPS",
  L"EDENRED",
  L"EDILIZIACROBATICA",
  L"EDITIONS DU SIGNE",
  L"EDUFORM ACTION",
  L"EGIDE",
  L"EGIDE BSA",
  L"EIFFAGE",
  L"EKINOPS",
  L"ELEC.STRASBOURG",
  L"ELECT. MADAGASCAR",
  L"ELIOR GROUP",
  L"ELIS",
  L"ELIX",
  L"EMBENTION",
  L"EMEIS",
  L"EMOVA GROUP",
  L"ENCRES DUBUIT",
  L"ENENSYS",
  L"ENERGISME",
  L"ENERGY SOLAR",
  L"ENGIE",
  L"ENOGIA",
  L"ENTECH",
  L"EO2",
  L"EQUASENS",
  L"ERAMET",
  L"ES VEDRA",
  L"ESSILORLUXOTTICA",
  L"ETHERO",
  L"EURASIA FONC INV",
  L"EURASIA GROUPE",
  L"EURAZEO",
  L"EUROAPI",
  L"EUROBIO-SCIENTIFIC",
  L"EUROFINS CEREP",
  L"EUROFINS SCIENT.",
  L"EUROLAND CORPORATE",
  L"EUROLOG CANOLA",
  L"EURONEXT",
  L"EUROPACORP",
  L"European Medical S",
  L"EUROPLASMA",
  L"EUTELSAT COMMUNIC.",
  L"EVERGREEN",
  L"EXACOMPTA CLAIREF.",
  L"EXAIL TECHNOLOGIES",
  L"EXEL INDUSTRIES",
  L"EXOSENS",
  L"EXPLOSIFS PROD.CHI",
  L"FACEPHI",
  L"FAIFEY INVEST",
  L"FD",
  L"FDJ UNITED",
  L"FERM.CAS.MUN.CANNE",
  L"FERMENTALG",
  L"FIDUCIAL OFF.SOL.",
  L"FIDUCIAL REAL EST.",
  L"FIGEAC AERO",
  L"FILL UP MEDIA",
  L"FIN.OUEST AFRICAIN",
  L"FINANCIERE MARJOS",
  L"FINAXO",
  L"FIPP",
  L"FIRSTCAUTION",
  L"FLEURY MICHON",
  L"FLORENTAISE",
  L"FNAC DARTY",
  L"FNPTECHNOLOGIESSA",
  L"FONCIERE 7 INVEST",
  L"FONCIERE INEA",
  L"FONCIERE VINDI",
  L"FONCIERE VOLTA",
  L"FORESTIERE EQUAT.",
  L"FORSEE POWER",
  L"FORVIA",
  L"FOUNTAINE PAJOT",
  L"FRANCAISE ENERGIE",
  L"FRANCE TOURISME",
  L"FREELANCE.COM",
  L"FREY",
  L"FSDV",
  L"G.A.I.",
  L"GALEO",
  L"GASCOGNE",
  L"GAUMONT",
  L"GEA GRENOBL.ELECT.",
  L"GECI INTL",
  L"GECINA",
  L"GENESIS",
  L"GENEURO",
  L"GENFIT",
  L"GENOWAY",
  L"GENSIGHT BIOLOGICS",
  L"GENSIGHT BSA",
  L"GENTLEMENS EQUITY",
  L"GETLINK SE",
  L"GEVELOT",
  L"GL EVENTS",
  L"GLASS TO POWER",
  L"GLOBAL PIELAGO",
  L"GOLD BY GOLD",
  L"GPE GROUP PIZZORNO",
  L"GRAINES VOLTZ",
  L"GROLLEAU",
  L"GROUPE CARNIVOR",
  L"GROUPE CRIT",
  L"GROUPE GUILLIN",
  L"GROUPE JAJ",
  L"GROUPE LDLC",
  L"GROUPE OKWIND",
  L"GROUPE PARTOUCHE",
  L"GROUPE PLUS-VALUES",
  L"GROUPE SFPI",
  L"GROUPE TERA",
  L"GROUPIMO",
  L"GTT",
  L"GUERBET",
  L"GUILLEMOT",
  L"HAFFNER ENERGY",
  L"HAFFNER ENERGY BSA",
  L"HAMILTON GLOBAL OP",
  L"HAULOTTE GROUP",
  L"HDF",
  L"HEALTHCARE ACTIVOS",
  L"HERIGE",
  L"HERMES INTL",
  L"HEXAOM",
  L"HF COMPANY",
  L"HIGH CO",
  L"HIPAY GROUP",
  L"HITECHPROS",
  L"HK",
  L"HOCHE BAINS L.BAIN",
  L"HOFFMANN",
  L"HOME CONCEPT",
  L"HOPENING",
  L"HOPIUM",
  L"HOPSCOTCH GROUPE",
  L"HOT.MAJESTIC CANNE",
  L"HOTELES BESTPRICE",
  L"HOTELIM",
  L"HOTELS DE PARIS",
  L"HOTL.IMMOB.NICE",
  L"HUNYVERS",
  L"HYDRAULIQUEHOLDING",
  L"HYDRO-EXPLOIT.",
  L"HYDROGEN REFUELING",
  L"I2S",
  L"IANTE INVESTMENTS",
  L"ICADE",
  L"ICAPE HOLDING",
  L"ID LOGISTICS GROUP",
  L"IDI",
  L"IDS",
  L"IGIS NEPTUNE",
  L"IKONISYS",
  L"IMALLIANCE",
  L"IMEON ENERGY",
  L"IMERYS",
  L"IMM.PARIS.PERLE",
  L"IMMERSION",
  L"IMMOB.DASSAULT",
  L"IMPLANET",
  L"IMPRIMERIE CHIRAT",
  L"IMPULSE FITNESS",
  L"INFOCLIP",
  L"INFOTEL",
  L"INMARK",
  L"INMOLECULE",
  L"INMOSUPA",
  L"INNATE PHARMA",
  L"INNELEC MULTIMEDIA",
  L"INNOVATIVE RFK SPA",
  L"INSTALLUX",
  L"INTEGRAGEN",
  L"INTEGRITAS VIAGER",
  L"INTERPARFUMS",
  L"INTEXA",
  L"INTRASENSE",
  L"INVENTIVA",
  L"INVIBES ADVERTSING",
  L"IPOSA PROPERTIES",
  L"IPSEN",
  L"IPSOS",
  L"ISPD",
  L"IT LINK",
  L"ITALY INNOVAZIONI",
  L"JACQUES BOGART",
  L"JACQUET METALS",
  L"JCDECAUX",
  L"KALEON",
  L"KALRAY",
  L"KAUFMAN ET BROAD",
  L"KERING",
  L"KERLINK",
  L"KEYRUS",
  L"KKO INTERNATIONAL",
  L"KLARSEN",
  L"KLARSEN BSA O",
  L"KLARSEN BSA P",
  L"KLEA HOLDING",
  L"KLEPIERRE",
  L"KOMPUESTOS",
  L"KUMULUS VAPE",
  L"L\'OREAL",
  L"LABO EUROMEDIS",
  L"LACROIX GROUP",
  L"LAGARDERE SA",
  L"LANSON-BCC",
  L"LARGO",
  L"LATECOERE",
  L"LAURENT-PERRIER",
  L"LDC",
  L"LEBON",
  L"LECTRA",
  L"LECTRA NV26",
  L"LEGRAND",
  L"LEPERMISLIBRE",
  L"LES HOTELS BAVEREZ",
  L"LEXIBOOK LINGUIST.",
  L"LHYFE",
  L"LIGHTON",
  L"LINEDATA SERVICES",
  L"LISI",
  L"LLEIDA",
  L"LNA SANTE",
  L"LOCASYSTEM INTL",
  L"LOGIC INSTRUMENT",
  L"LOMBARD ET MEDOT",
  L"LOUIS HACHETTE",
  L"LUCIBEL",
  L"LUMIBIRD",
  L"LVMH",
  L"MAAT PHARMA",
  L"MACOMPTA.FR",
  L"MAGILLEM",
  L"MAISON CLIO BLUE",
  L"MAISON POMMERY",
  L"MAISONS DU MONDE",
  L"MAKING SCIENCE",
  L"MALTERIES FCO-BEL.",
  L"MANITOU BF",
  L"MAQ ADMON. URBANAS",
  L"MARE NOSTRUM",
  L"MAROC TELECOM",
  L"MASTRAD",
  L"MASTRAD BS29",
  L"MAUNA KEA BSA",
  L"MAUNA KEA TECH",
  L"MAUREL ET PROM",
  L"MBWS",
  L"MEDIA 6",
  L"MEDIA LAB",
  L"MEDIAN TECH BSA",
  L"MEDIANTECHNOLOGIES",
  L"MEDINCELL",
  L"MEMSCAP REGPT",
  L"MERCIALYS",
  L"MERIDIA RE IV",
  L"MERSEN",
  L"METAVISIO",
  L"METHANOR",
  L"METRICS IN BALANCE",
  L"METROPOLE TV",
  L"MEXEDIA",
  L"MG INTERNATIONAL",
  L"MGI DIGITAL GRAPHI",
  L"MICHELIN",
  L"MIGUET ET ASSOCIES",
  L"MILIBOO",
  L"MINT",
  L"MON COURTIER ENERG",
  L"MONCEY (FIN.) NOM.",
  L"MONTEA",
  L"MONTEPINO LOGISTIC",
  L"MOULINVEST",
  L"MR BRICOLAGE",
  L"MUNIC",
  L"MUTTER VENTURES",
  L"MYHOTELMATCH",
  L"NACON",
  L"NAMR",
  L"NANOBIOTIX",
  L"NEOLIFE",
  L"NEOVACS",
  L"NETGEM",
  L"NETMEDIA GROUP",
  L"NEURONES",
  L"NEXANS",
  L"NEXITY",
  L"NEXTEDIA",
  L"NFL BIOSCIENCES",
  L"NICOX",
  L"NICOX BSA",
  L"NORTEM BIOGROUP",
  L"North Atlantic En.",
  L"NOVACYT",
  L"NOVATECH IND.",
  L"NR21",
  L"NRJ GROUP",
  L"NSC GROUPE",
  L"NSE",
  L"OBIZ",
  L"OCTOPUS BIOSAFETY",
  L"ODIOT S.A.",
  L"ODYSSEE TECHNO",
  L"OENEO",
  L"OK PROPERTIES",
  L"OMER-DECUGIS & CIE",
  L"ONCODESIGN PM",
  L"ONE EXP BSA 2025 2",
  L"ONE EXPERIENCE",
  L"ONLINEFORMAPRO",
  L"ONWARD MEDICAL",
  L"OPMOBILITY",
  L"ORANGE",
  L"ORBIS PROPERTIES",
  L"ORDISSIMO",
  L"OREGE",
  L"ORINOQUIA",
  L"OSE IMMUNO",
  L"OVH",
  L"PACTE NOVATION",
  L"PAREF",
  L"PARROT",
  L"PARX MATERIALS NV",
  L"PASSAT",
  L"PATRIMOINE ET COMM",
  L"PAULIC MEUNERIE",
  L"PERNOD RICARD",
  L"PERRIER (GERARD)",
  L"PERSEIDA RENTA",
  L"PET SERVICE",
  L"PEUGEOT INVEST",
  L"PHONE WEB",
  L"PHOTONIKE CAPITAL",
  L"PIERRE VAC BSA ACT",
  L"PIERRE VAC BSA CRE",
  L"PIERRE VACANCES",
  L"PISCINES DESJOYAUX",
  L"PLACOPLATRE",
  L"PLANISWARE",
  L"PLANT ADVANCED",
  L"PLANT ADVANCED BS",
  L"PLAST.VAL LOIRE",
  L"PLUXEE",
  L"POUJOULAT",
  L"POULAILLON",
  L"POXEL",
  L"PREATONI GROUP",
  L"PRECIA",
  L"PREDILIFE",
  L"PRELUDE S.A",
  L"PRISMAFLEX INTL",
  L"PROACTIS SA",
  L"PRODWAYS GROUP",
  L"PROLOGUE",
  L"PROP.IMMEUBLES",
  L"PUBLICIS GROUPE SA",
  L"PULLUP ENTERTAIN",
  L"QUADIENT",
  L"QUANTUM GENOMICS",
  L"QWAMPLIFY",
  L"RACING FORCE",
  L"RAMSAY GEN SANTE",
  L"RAPID NUTRITION",
  L"REALITES",
  L"REBIRTH",
  L"REMY COINTREAU",
  L"RENAULT",
  L"RES GESTAE SOCIMI",
  L"REWORLD MEDIA",
  L"REXEL",
  L"RIBER",
  L"RISING STONE",
  L"ROBERTET",
  L"ROBERTET CDV 87",
  L"ROBERTET CI",
  L"ROCHE BOBOIS",
  L"ROCTOOL",
  L"ROUGIER S.A.",
  L"RUBIS",
  L"S.E.B.",
  L"SAFE",
  L"SAFRAN",
  L"SAGAX REAL ESTATE",
  L"SAINT GOBAIN",
  L"SAINT GOBAIN NV26",
  L"SAINT JEAN GROUPE",
  L"SAJA REAL ESTATE",
  L"SAMSE",
  L"SANOFI",
  L"SANOFI NV26",
  L"SAPMER",
  L"SARTORIUS STED BIO",
  L"SAVENCIA",
  L"SAVONNERIE NYONS",
  L"SCBSM",
  L"SCEMI",
  L"SCHNEIDER ELECTRIC",
  L"SCIENTIA SCHOOL",
  L"SCOR SE",
  L"SECHE ENVIRONNEM.",
  L"SEGRO PLC",
  L"SEIF SPA",
  L"SELCODIS",
  L"SELECTIRENTE",
  L"SEMCO TECHNOLOGIES",
  L"SENSORION",
  L"SEQUA PETROLEUM NV",
  L"SERGEFERRARI GROUP",
  L"SES",
  L"SHOWROOMPRIVE",
  L"SIDETRADE",
  L"SIGNAUX GIROD",
  L"SIPARIO MOVIES",
  L"SIRIUS MEDIA",
  L"SMAIO",
  L"SMALTO",
  L"SMALTO BSA",
  L"SMCP",
  L"SMTPC",
  L"SOC FRANC CASINOS",
  L"SOCIETE GENERALE",
  L"SODEXO",
  L"SODITECH",
  L"SOGECLAIR",
  L"SOITEC",
  L"SOLOCAL GROUP",
  L"SOLUTIONS 30 SE",
  L"SOLVAY",
  L"SOPRA STERIA GROUP",
  L"SPARTOO",
  L"SPEED RABBIT PIZZA",
  L"SPIE",
  L"SPINEGUARD",
  L"SPINEWAY",
  L"ST DUPONT",
  L"STEF",
  L"STELLANTIS NV",
  L"STIF",
  L"STMICROELECTRONICS",
  L"STRADIM ESPAC.FIN",
  L"STREAMWIDE",
  L"STREIT MECANIQUE",
  L"STUDENT PROPERTY",
  L"SWORD GROUP",
  L"SYNERGIE",
  L"TBSO",
  L"TD BSA 2025-2",
  L"TD BSA 2025-3",
  L"TECHNIP ENERGIES",
  L"TELEPERFORMANCE",
  L"TELEVERBIER",
  L"TELEVISTA",
  L"TERACT",
  L"TERACT BS",
  L"TF1",
  L"TF1 NV26",
  L"TFF GROUP",
  L"THALES",
  L"THE AZUR SELECTION",
  L"THERACLION",
  L"THERMADOR GROUPE",
  L"THX PHARMA",
  L"TIKEHAU CAPITAL",
  L"TITAN S.A.",
  L"TME PHARMA",
  L"TONNER DRONES",
  L"TONNER DRONES BSA",
  L"TOOSLA",
  L"TOTALENERGIES",
  L"TotalEnergiesGabon",
  L"TOUAX",
  L"TOUR EIFFEL",
  L"TRACTIAL",
  L"TRANSGENE",
  L"TRIGANO",
  L"TRILOGIQ",
  L"TROC ILE",
  L"TXCOM",
  L"U10 CORP",
  L"UBISOFT ENTERTAIN",
  L"UCAPITAL GLOBAL",
  L"UMALIS GROUP",
  L"UNIBAIL-RODAMCO-WE",
  L"UNION TECH.INFOR.",
  L"UNITI",
  L"UPERGY",
  L"URCOLESA",
  L"UV GERMI",
  L"VALBIOTIS",
  L"VALEO",
  L"VALERIO TX",
  L"VALLOUREC",
  L"VALLOUREC BSA 21",
  L"VALNEVA",
  L"VANDOR REAL ESTATE",
  L"VANTIVA",
  L"VAZIVA",
  L"VAZIVA BSA",
  L"VENTE UNIQUE.COM",
  L"VEOLIA ENVIRON.",
  L"VEOM GROUP",
  L"VERALLIA",
  L"VERGNET",
  L"VERIMATRIX",
  L"VETOQUINOL",
  L"VIALIFE",
  L"VICAT",
  L"VICSO",
  L"VIEL ET COMPAGNIE",
  L"VINCI",
  L"VINPAI",
  L"VIRBAC",
  L"VIRIDIEN",
  L"VIRTUALWARE",
  L"VITURA",
  L"VIVENDI SE",
  L"VOGO",
  L"VOLTALIA",
  L"VOYAGEURS DU MONDE",
  L"VREF SEVILLE",
  L"Vusion",
  L"WAGA ENERGY",
  L"WALLIX",
  L"WATERA",
  L"WAVESTONE",
  L"WE.CONNECT",
  L"WEACCESS GROUP",
  L"WELLNESS",
  L"WENDEL",
  L"WEYA",
  L"WINAMP GROUP",
  L"WINFARM",
  L"WITBE",
  L"WORLDLINE",
  L"WORLDLINE DS",
  L"X-FAB",
  L"XILAM ANIMATION",
  L"YOUNITED FIN. WARR",
  L"YOUNITED FINANCIAL",
  L"ZCCM",
  L"ZCI LIMITED"
} ;

static const vector <wstring> MNEMOS = {
  L"",
  L"EU%5EPX1",
  L"EU%5EPX4",
  L"DBI%5EDAX",
  L"FT%5EUKX",
  L"SPI%5ESP500",
  L"DJI%5EI%5CDJI",
  L"NI%5EI%5CCOMP",
  L"",
  L"FX%5EEURUSD",
  L"FX%5EEURJPY",
  L"FX%5EEURGBP",
  L"FX%5EEURCHF",
  L"",
  L"AL2SI",
  L"74SW",
  L"AB",
  L"ABBS",
  L"ABSBS",
  L"ABBSA",
  L"ABCA",
  L"ABEO",
  L"ABNX",
  L"ABVX",
  L"ABLD",
  L"ABO",
  L"ACAN",
  L"AC",
  L"ALALO",
  L"EOS",
  L"ALATI",
  L"MLACT",
  L"ALDV",
  L"ALARF",
  L"ADOC",
  L"ADOBS",
  L"ALADO",
  L"ADP",
  L"ALDUX",
  L"ALDVI",
  L"ALAVI",
  L"AELIS",
  L"AKOM",
  L"ALAFY",
  L"MLAGI",
  L"MLAA",
  L"MLAGP",
  L"ALAGP",
  L"ALAGR",
  L"AF",
  L"AI",
  L"AIR",
  L"ALAIR",
  L"AKW",
  L"AAA",
  L"CDA",
  L"ALO",
  L"LTA",
  L"ALTA",
  L"AREIT",
  L"ATE",
  L"ALORA",
  L"MLALV",
  L"ALAMA",
  L"MLAAH",
  L"ALMIB",
  L"AMUN",
  L"MLAIG",
  L"ANTIN",
  L"APAM",
  L"MLASO",
  L"ALAQU",
  L"ARAMI",
  L"MT",
  L"ALJXR",
  L"ALCUR",
  L"MLARD",
  L"ARDO",
  L"MLARE",
  L"ARG",
  L"MLARI",
  L"AKE",
  L"MLARO",
  L"ARTE",
  L"PRC",
  L"ARTO",
  L"ARVEN",
  L"ARVBS",
  L"MLAEM",
  L"ASY",
  L"MLAST",
  L"ALATA",
  L"ATEME",
  L"ATLD",
  L"ALTAO",
  L"ATO",
  L"AUB",
  L"ALAUD",
  L"AUGR",
  L"AURE",
  L"AVT",
  L"CS",
  L"CSNV",
  L"AYV",
  L"MLAZL",
  L"MLAAT",
  L"ALBKK",
  L"BAIN",
  L"BALYO",
  L"BUI",
  L"MLBAR",
  L"BASS",
  L"BLC",
  L"MLBBO",
  L"BEN",
  L"ALDBL",
  L"BB",
  L"BIG",
  L"ALBLD",
  L"MLBIM",
  L"MLNOX",
  L"ALTUV",
  L"BIM",
  L"ALBPS",
  L"BPYBS",
  L"BPBS",
  L"BIBS3",
  L"BIOS",
  L"ALBIO",
  L"BLEE",
  L"MLBSP",
  L"ALBLU",
  L"BNP",
  L"ALBOA",
  L"MLONE",
  L"BOI",
  L"BOL",
  L"BON",
  L"MLBON",
  L"ALBOO",
  L"ALBOU",
  L"BSD",
  L"EN",
  L"ENNV",
  L"ALBPK",
  L"BVI",
  L"BUR",
  L"CAT31",
  L"ALCAB",
  L"ALCAF",
  L"ALIBR",
  L"IBRKA",
  L"IBRKB",
  L"CBDG",
  L"CAP",
  L"ALCPB",
  L"CPBBS",
  L"ALCRB",
  L"CARM",
  L"CA",
  L"CVX",
  L"COBS1",
  L"COBS3",
  L"CO",
  L"ALCAT",
  L"ALCIS",
  L"ALCBI",
  L"CBBSA",
  L"CBSAB",
  L"CBOT",
  L"ALCGM",
  L"ALCLS",
  L"CYAD",
  L"CFI",
  L"MLCFM",
  L"MLCFD",
  L"ALCWE",
  L"CHSR",
  L"MLCHE",
  L"CDI",
  L"ALCBX",
  L"MLCMB",
  L"CRI",
  L"ALCLA",
  L"CLARI",
  L"MLCMG",
  L"COFA",
  L"ALCOF",
  L"ALCOG",
  L"COH",
  L"ALCOI",
  L"MLCLP",
  L"ODET",
  L"MLMFI",
  L"MLCNT",
  L"MLLCB",
  L"MLCOE",
  L"MLCOR",
  L"MLCOT",
  L"COTY",
  L"MLCOU",
  L"COUR",
  L"COV",
  L"COVH",
  L"CRAP",
  L"CRAV",
  L"CRBP2",
  L"CIV",
  L"CRLA",
  L"CRLO",
  L"CMO",
  L"CNDF",
  L"CCN",
  L"CAF",
  L"CRSU",
  L"CRTO",
  L"ACA",
  L"ALCJ",
  L"CJBS",
  L"CROS",
  L"ALDLS",
  L"MLDAM",
  L"ALDAR",
  L"BN",
  L"AM",
  L"DSY",
  L"ALDBT",
  L"DBV",
  L"DEEZR",
  L"DEEZW",
  L"DKUPL",
  L"ALDEL",
  L"ALDLT",
  L"DBG",
  L"ALDEV",
  L"ALDMS",
  L"ALDNX",
  L"DPAM",
  L"ALDOL",
  L"ALDNE",
  L"ALDRV",
  L"BNBS",
  L"DRVBS",
  L"DRNBS",
  L"MLDYN",
  L"ALAGO",
  L"EFG",
  L"MLEAS",
  L"MLEDR",
  L"MLEAV",
  L"ALECO",
  L"ALESA",
  L"EDEN",
  L"ALEAC",
  L"MLEDS",
  L"MLEFA",
  L"ALGID",
  L"GIDBS",
  L"FGR",
  L"EKI",
  L"ELEC",
  L"EEM",
  L"ELIOR",
  L"ELIS",
  L"MLERH",
  L"MLUAV",
  L"EMEIS",
  L"ALEMV",
  L"ALDUB",
  L"ALNN6",
  L"ALNRG",
  L"ALETC",
  L"ENGI",
  L"ALENO",
  L"ALESE",
  L"ALEO2",
  L"EQS",
  L"ERA",
  L"MLESV",
  L"EL",
  L"ALENT",
  L"EFI",
  L"ALEUA",
  L"RF",
  L"EAPI",
  L"ALERS",
  L"ALECR",
  L"ERF",
  L"ALERO",
  L"MLCAN",
  L"ENX",
  L"ALECP",
  L"ALEMS",
  L"ALEUP",
  L"ETL",
  L"EGR",
  L"ALEXA",
  L"EXA",
  L"EXE",
  L"EXENS",
  L"EXPL",
  L"ALPHI",
  L"MLECE",
  L"MLFDV",
  L"FDJU",
  L"FCMC",
  L"ALGAE",
  L"SACI",
  L"ORIA",
  L"FGA",
  L"ALFUM",
  L"FOAF",
  L"FINM",
  L"MLFXO",
  L"FIPP",
  L"MLFIR",
  L"ALFLE",
  L"ALFLO",
  L"FNAC",
  L"MLFNP",
  L"LEBL",
  L"INEA",
  L"MLVIN",
  L"SPEL",
  L"FORE",
  L"ALFOR",
  L"FRVIA",
  L"ALFPC",
  L"FDE",
  L"MLFTI",
  L"ALFRE",
  L"FREY",
  L"FSDV",
  L"MLGAI",
  L"MLGAL",
  L"ALBI",
  L"GAM",
  L"GEA",
  L"ALGEC",
  L"GFC",
  L"MLGNS",
  L"GNRO",
  L"GNFT",
  L"ALGEN",
  L"SIGHT",
  L"SIGBS",
  L"MLGEQ",
  L"GET",
  L"ALGEV",
  L"GLO",
  L"MLGL",
  L"MLNDG",
  L"ALGLD",
  L"GPE",
  L"GRVO",
  L"ALGRO",
  L"MLGRC",
  L"CEN",
  L"ALGIL",
  L"GJAJ",
  L"ALLDL",
  L"ALOKW",
  L"PARP",
  L"MLPVG",
  L"SFPI",
  L"ALGTR",
  L"ALIMO",
  L"GTT",
  L"GBT",
  L"GUI",
  L"ALHAF",
  L"HAFBS",
  L"ALHGO",
  L"PIG",
  L"HDF",
  L"MLHAY",
  L"ALHRG",
  L"RMS",
  L"ALHEX",
  L"ALHF",
  L"HCO",
  L"ALHYP",
  L"ALHIT",
  L"MLHK",
  L"MLHBB",
  L"ALHGR",
  L"MLHCF",
  L"MLHPE",
  L"ALHPI",
  L"ALHOP",
  L"MLHMC",
  L"MLHBP",
  L"MLHOT",
  L"HDP",
  L"MLHIN",
  L"ALHUN",
  L"MLHYD",
  L"MLHYE",
  L"ALHRS",
  L"ALI2S",
  L"MLINT",
  L"ICAD",
  L"ALICA",
  L"IDL",
  L"IDIP",
  L"MLIDS",
  L"MLABC",
  L"ALIKO",
  L"MLIML",
  L"MLIME",
  L"NK",
  L"MLIPP",
  L"ALIMR",
  L"IMDA",
  L"ALIMP",
  L"MLIMP",
  L"MLIFS",
  L"MLIFC",
  L"INF",
  L"MLINM",
  L"MLINA",
  L"MLISP",
  L"IPH",
  L"ALINN",
  L"MLIRF",
  L"ALLUX",
  L"ALINT",
  L"MLVIE",
  L"ITP",
  L"ITXT",
  L"ALINS",
  L"IVA",
  L"ALINV",
  L"MLIPO",
  L"IPN",
  L"IPS",
  L"ALISP",
  L"ALITL",
  L"MLITN",
  L"JBOG",
  L"JCQ",
  L"DEC",
  L"ALKLN",
  L"ALKAL",
  L"KOF",
  L"KER",
  L"ALKLK",
  L"ALKEY",
  L"ALKKO",
  L"ALKLA",
  L"KLBSO",
  L"KLBSP",
  L"ALKLH",
  L"LI",
  L"ALKOM",
  L"ALVAP",
  L"OR",
  L"ALEMG",
  L"LACR",
  L"MMB",
  L"ALLAN",
  L"ALLGO",
  L"LAT",
  L"LPE",
  L"LOUP",
  L"ALBON",
  L"LSS",
  L"LSSNV",
  L"LR",
  L"ALLPL",
  L"ALLHB",
  L"ALLEX",
  L"LHYFE",
  L"ALTAI",
  L"LIN",
  L"FII",
  L"ALLLN",
  L"LNA",
  L"MLLOI",
  L"ALLOG",
  L"MLCAC",
  L"ALHG",
  L"ALUCI",
  L"LBIRD",
  L"MC",
  L"MAAT",
  L"MLMCA",
  L"MLMGL",
  L"MLCLI",
  L"POMRY",
  L"MDM",
  L"ALMKS",
  L"MALT",
  L"MTU",
  L"MLMAQ",
  L"ALMAR",
  L"IAM",
  L"ALMAS",
  L"MASBS",
  L"MKTBS",
  L"ALMKT",
  L"MAU",
  L"MBWS",
  L"EDI",
  L"MLLAB",
  L"MDTBS",
  L"ALMDT",
  L"MEDCL",
  L"MEMS",
  L"MERY",
  L"MLMIV",
  L"MRN",
  L"ALTHO",
  L"ALMET",
  L"MLMIB",
  L"MMT",
  L"ALMEX",
  L"ALMGI",
  L"ALMDG",
  L"ML",
  L"MLNMA",
  L"ALMLB",
  L"ALMIN",
  L"ALMCE",
  L"FMONC",
  L"MONT",
  L"MLMTP",
  L"ALMOU",
  L"ALMRB",
  L"ALMUN",
  L"MLMUT",
  L"MHM",
  L"NACON",
  L"ALNMR",
  L"NANO",
  L"ALNLF",
  L"ALNEV",
  L"ALNTG",
  L"ALNMG",
  L"NRO",
  L"NEX",
  L"NXI",
  L"ALNXT",
  L"ALNFL",
  L"ALCOX",
  L"NICBS",
  L"MLBIO",
  L"NAE",
  L"ALNOV",
  L"MLNOV",
  L"NR21",
  L"NRG",
  L"ALNSC",
  L"ALNSE",
  L"ALBIZ",
  L"MLOCT",
  L"MLODT",
  L"ALODY",
  L"SBT",
  L"MLOKP",
  L"ALODC",
  L"ALOPM",
  L"ONEBS",
  L"ALEXP",
  L"MLONL",
  L"ONWD",
  L"OPM",
  L"ORA",
  L"MLORB",
  L"ALORD",
  L"OREGE",
  L"MLORQ",
  L"OSE",
  L"OVH",
  L"MLPAC",
  L"PAR",
  L"PARRO",
  L"MLPRX",
  L"ALPAS",
  L"PAT",
  L"ALPAU",
  L"RI",
  L"PERR",
  L"MLPER",
  L"ALPET",
  L"PEUG",
  L"MLPHW",
  L"MLPHO",
  L"VACBS",
  L"VACBT",
  L"VAC",
  L"ALPDX",
  L"MLPLC",
  L"PLNW",
  L"ALPAT",
  L"PATBS",
  L"PVL",
  L"PLX",
  L"ALPJT",
  L"ALPOU",
  L"POXEL",
  L"MLPRG",
  L"ALPM",
  L"ALPRE",
  L"MLPRE",
  L"ALPRI",
  L"PROAC",
  L"ALPWG",
  L"ALPRG",
  L"MLPRI",
  L"PUB",
  L"ALPUL",
  L"QDT",
  L"ALQGC",
  L"ALQWA",
  L"ALRFG",
  L"GDS",
  L"ALRPD",
  L"ALREA",
  L"ALREB",
  L"RCO",
  L"RNO",
  L"MLJDL",
  L"ALREW",
  L"RXL",
  L"ALRIB",
  L"ALRIS",
  L"RBT",
  L"CBR",
  L"CBE",
  L"RBO",
  L"ALROC",
  L"ALRGR",
  L"RUI",
  L"SK",
  L"ALSAF",
  L"SAF",
  L"MLSAG",
  L"SGO",
  L"SGONV",
  L"SABE",
  L"MLSJA",
  L"SAMS",
  L"SAN",
  L"SANNV",
  L"ALMER",
  L"DIM",
  L"SAVE",
  L"MLSDN",
  L"CBSM",
  L"MLCMI",
  L"SU",
  L"MLSCI",
  L"SCR",
  L"SCHP",
  L"SGRO",
  L"ALSEI",
  L"SLCO",
  L"SELER",
  L"ALSEM",
  L"ALSEN",
  L"MLSEQ",
  L"SEFER",
  L"SESG",
  L"SRP",
  L"ALBFR",
  L"ALGIR",
  L"ALIE",
  L"ALSRS",
  L"ALSMA",
  L"MLSML",
  L"SMLBS",
  L"SMCP",
  L"ALTPC",
  L"SFCA",
  L"GLE",
  L"SW",
  L"ALSEC",
  L"ALSOG",
  L"SOI",
  L"LOCAL",
  L"S30",
  L"SOLB",
  L"SOP",
  L"ALSPT",
  L"MLSRP",
  L"SPIE",
  L"ALSGD",
  L"ALSPW",
  L"DPT",
  L"STF",
  L"STLAP",
  L"ALSTI",
  L"STMPA",
  L"ALSAS",
  L"ALSTW",
  L"MLSTR",
  L"MLSPI",
  L"SWP",
  L"SDG",
  L"TBSO",
  L"TDBS2",
  L"TDBS3",
  L"TE",
  L"TEP",
  L"TVRB",
  L"MLVST",
  L"TRACT",
  L"TERBS",
  L"TFI",
  L"TFINV",
  L"TFF",
  L"HO",
  L"MLAZR",
  L"ALTHE",
  L"THEP",
  L"ALTHX",
  L"TKO",
  L"TITC",
  L"ALTME",
  L"ALTD",
  L"TDBS",
  L"ALTOO",
  L"TTE",
  L"EC",
  L"ALTOU",
  L"EIFF",
  L"ALTRA",
  L"TNG",
  L"TRI",
  L"ALTRI",
  L"MLTRO",
  L"ALTXC",
  L"ALU10",
  L"UBI",
  L"MLALE",
  L"MLUMG",
  L"URW",
  L"FPG",
  L"ALUNT",
  L"ALUPG",
  L"MLURC",
  L"ALUVI",
  L"ALVAL",
  L"FR",
  L"ALVIO",
  L"VK",
  L"VKBS",
  L"VLA",
  L"MLVRE",
  L"VANTI",
  L"ALVAZ",
  L"VAZBS",
  L"ALVU",
  L"VIE",
  L"ALVG",
  L"VRLA",
  L"ALVER",
  L"VMX",
  L"VETO",
  L"ALVIA",
  L"VCT",
  L"MLVIC",
  L"VIL",
  L"DG",
  L"ALVIN",
  L"VIRP",
  L"VIRI",
  L"ALVIR",
  L"VTR",
  L"VIV",
  L"ALVGO",
  L"VLTSA",
  L"ALVDM",
  L"MLVRF",
  L"VU",
  L"WAGA",
  L"ALLIX",
  L"ALWTR",
  L"WAVE",
  L"ALWEC",
  L"MLWEA",
  L"MLWRS",
  L"MF",
  L"MLWEY",
  L"ALWIN",
  L"ALWF",
  L"ALWIT",
  L"WLN",
  L"WLNDS",
  L"XFAB",
  L"ALXIL",
  L"YOUNW",
  L"YOUNI",
  L"MLZAM",
  L"CV"
} ;

// STRUCTURES

struct EFFECTEURS {

  // Contrôles de la section "effecteurs" (paramètres de calcul)

  HWND comboPeriode = NULL ; // Sélection de la période (1 mois, 2 mois...)
  HWND btnToggle    = NULL ; // Bascule graphe durée / effecteurs
  HWND comboType    = NULL ; // Type de graphe (ligne, chandelier, barre...)
  HWND lblNombre    = NULL ; // Label "Nombre"
  HWND lblValeur    = NULL ; // Label "Valeur"
  HWND editNombre   = NULL ; // Saisie du nombre de titres
  HWND editValeur   = NULL ; // Saisie de la valeur unitaire
  HWND btnCalc      = NULL ; // Bouton "Calcul"
  HWND chkAvant5Ans = NULL ; // Checkbox PEA < 5 ans (taux imposition différent)
  HWND chkTTF       = NULL ; // Checkbox TTF (taxe sur transactions financières)
  vector <HWND> GetControls () const {
    return { comboPeriode, btnToggle, comboType, lblNombre, lblValeur, editNombre, editValeur, btnCalc, chkAvant5Ans, chkTTF } ;
  }
} ;

struct RESULTATS {

  // Contrôles de la section résultats de calcul
  // Un seul STATIC multilignes (\r\n) pour afficher les 6 lignes de résultats
  // Cliquer sur ce contrôle masque les résultats (appel MasqueResultats)

  HWND lblAll = NULL ;
  vector <HWND> GetControls () const {
    return { lblAll } ;
  }
} ;

struct InstanceConfig {

  // Configuration d'une instance sauvegardée dans le registre

  INT index = 1 ;             // Index de l'indice dans LABELS/MNEMOS
  INT x     = CW_USEDEFAULT ; // Position X de la fenêtre
  INT y     = CW_USEDEFAULT ; // Position Y de la fenêtre
} ;

// FORWARD DECLARATIONS

// Nécessaire car UpdateLayout() appelle ces deux fonctions définies après
/// VOID DownloadAndDisplayImage (const wstring & mnemo, BOOL forDuree) ;
/// static INT DoLayout (LayoutMode mode, HFONT hFont = NULL) ;

// VARIABLES GLOBALES

static UINT        g_dpi                 = 96 ;    // DPI courant de la fenêtre
static HFONT       g_hSmallFont          = NULL ;  // Police Segoe UI Bold 15px
static EFFECTEURS  g_effecteurs          = {} ;    // Handles des contrôles effecteurs
static RESULTATS   g_result              = {} ;    // Handle du contrôle résultats
static HWND        g_hList               = NULL ;  // Combobox principale
static HWND        g_hIntraday           = NULL ;  // STATIC image intraday
static HWND        g_hDuree              = NULL ;  // STATIC image graphe durée
static HWND        g_hwnd                = NULL ;  // Handle de la fenêtre principale
static HWND        g_hwndOwner           = NULL ;  // Fenêtre propriétaire cachée (masque barre des tâches)
static INT         g_currentIndex        = 0 ;     // Index sélectionné dans la combobox
static HeightState g_state               = STATE_COLLAPSED ;
static wstring     g_currentMnemo        = {} ;    // Mnémonique de l'indice affiché
static BOOL        g_avant5AnsChecked    = FALSE ;
static BOOL        g_ttfChecked          = FALSE ;
static ULONG_PTR   g_gdiplusToken        = 0 ;
static HBITMAP     g_hBitmap_Intraday    = NULL ;
static HBITMAP     g_hBitmap_Duree       = NULL ;
static BOOL        g_GraphDureeOuOptions = TRUE ;  // TRUE = graphe durée, FALSE = effecteurs
static INT         g_instanceId          = 0 ;     // Numéro de cette instance (0..MAX_INSTANCES-1)
static BOOL        g_isPrimaryInstance   = FALSE ; // TRUE = instance principale

// Handle du mutex de vie de cette instance
// Maintenu ouvert pendant toute la durée de vie du processus
// Sa fermeture signale aux autres instances que celle-ci est morte
static HANDLE      g_hInstanceMutex      = NULL ;

// Flag de suppression volontaire de l'instance
// TRUE  = suppression → WM_DESTROY ne sauvegarde PAS la config
// FALSE = fermeture normale → WM_DESTROY sauvegarde la config
static BOOL        g_deletingInstance    = FALSE ;

// MUTEX — GESTION DES INSTANCES VIVANTES

static HANDLE AcquireRegistryMutex (VOID) {

  // Acquiert le mutex d'accès exclusif au registre partagé
  // Timeout 5 secondes pour éviter un blocage infini
  // Retourne NULL en cas d'échec

////   if (Debug) LogTrace (__FUNCTION__) ;
  HANDLE hMutex = CreateMutexW (NULL, FALSE, MUTEX_REGISTRY.c_str ()) ;
  if ( ! hMutex) return NULL ;
  if (WaitForSingleObject (hMutex, 5000) == WAIT_OBJECT_0)
    return hMutex ;
  CloseHandle (hMutex) ;
  return NULL ;
}

static VOID ReleaseRegistryMutex (HANDLE hMutex) {

  // Relâche et ferme le mutex registre

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  if (hMutex) {
    ReleaseMutex (hMutex) ;
    CloseHandle  (hMutex) ;
  }
}

static BOOL IsInstanceAlive (INT instanceId) {

  // Teste si une instance N est vivante en tentant d'ouvrir son mutex nommé
  // Succès → vivante / Échec → morte

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  wstring name  = MUTEX_INSTANCE + to_wstring (instanceId) ;
  HANDLE hMutex = OpenMutexW (SYNCHRONIZE, FALSE, name.c_str ()) ;
  if (hMutex) {
    CloseHandle (hMutex) ;
    return TRUE ;
  }
  return FALSE ;
}

static HANDLE CreateInstanceMutex (INT instanceId) {

  // Crée le mutex de vie de cette instance
  // Maintenu ouvert jusqu'à WM_DESTROY

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  wstring name = MUTEX_INSTANCE + to_wstring (instanceId) ;
  return CreateMutexW (NULL, TRUE, name.c_str ()) ;
}

static INT FindFreeInstanceSlot (VOID) {

  // Cherche le premier slot libre (sans mutex vivant) parmi MAX_INSTANCES
  // Retourne -1 si tous les slots sont occupés

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  for (INT i = 0 ; i < MAX_INSTANCES ; i ++) {
    if ( ! IsInstanceAlive (i))
      return i ;
  }
  return -1 ;
}

// REGISTRE

static HKEY OpenAppKey (REGSAM access) {

  // Ouvre ou crée la clé registre de l'application

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  HKEY hKey = NULL ;
  RegCreateKeyExW (HKEY_CURRENT_USER, REG_KEY.c_str (), 0, NULL, REG_OPTION_NON_VOLATILE, access, NULL, & hKey, NULL) ;
  return hKey ;
}

static DWORD RegReadDword (HKEY hKey, const wstring & name, DWORD defaultVal) {

  // Lit un DWORD — retourne defaultVal si absent

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  DWORD val  = 0 ;
  DWORD size = sizeof (DWORD) ;
  DWORD type = REG_DWORD ;
  if (RegQueryValueExW (hKey, name.c_str (), NULL, & type, (LPBYTE) & val, & size) == ERROR_SUCCESS)
    return val ;
  return defaultVal ;
}

static VOID RegWriteDword (HKEY hKey, const wstring & name, DWORD val) {

  // Écrit un DWORD

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  RegSetValueExW (hKey, name.c_str (), 0, REG_DWORD, (const BYTE *) & val, sizeof (DWORD)) ;
}

static VOID SaveInstanceConfig (VOID) {

  // Sauvegarde la configuration de cette instance dans le registre
  // Appelée sur WM_MOVE, CBN_SELCHANGE et WM_DESTROY (fermeture normale)
  // NON appelée si g_deletingInstance == TRUE

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  HANDLE hRegMutex = AcquireRegistryMutex () ;
  HKEY hKey = OpenAppKey (KEY_WRITE) ;
  if (hKey) {
    wstring prefix = L"Instance" + to_wstring (g_instanceId) + L"_" ;
    RECT rc = {} ;
    if (g_hwnd) GetWindowRect (g_hwnd, & rc) ;
    RegWriteDword (hKey, prefix + L"Index", (DWORD) g_currentIndex) ;
    RegWriteDword (hKey, prefix + L"X",     (DWORD) rc.left) ;
    RegWriteDword (hKey, prefix + L"Y",     (DWORD) rc.top) ;
    RegCloseKey (hKey) ;
  }
  ReleaseRegistryMutex (hRegMutex) ;
}

static VOID DeleteInstanceConfig (INT instanceId) {

  // Supprime les entrées registre de cette instance
  // Appelée lors d'une suppression volontaire — le slot est libéré

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  HANDLE hRegMutex = AcquireRegistryMutex () ;
  HKEY hKey = OpenAppKey (KEY_WRITE) ;
  if (hKey) {
    wstring prefix = L"Instance" + to_wstring (instanceId) + L"_" ;
    RegDeleteValueW (hKey, (prefix + L"Index").c_str ()) ;
    RegDeleteValueW (hKey, (prefix + L"X").c_str ()) ;
    RegDeleteValueW (hKey, (prefix + L"Y").c_str ()) ;
    RegCloseKey (hKey) ;
  }
  ReleaseRegistryMutex (hRegMutex) ;
}

static InstanceConfig LoadInstanceConfig (INT instanceId) {

  // Lit la configuration d'une instance depuis le registre

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  InstanceConfig cfg ;
  HANDLE hRegMutex = AcquireRegistryMutex () ;
  HKEY hKey = OpenAppKey (KEY_READ) ;
  if (hKey) {
    wstring prefix = L"Instance" + to_wstring (instanceId) + L"_" ;
    cfg.index = (INT) RegReadDword (hKey, prefix + L"Index", 1) ;
    cfg.x     = (INT) RegReadDword (hKey, prefix + L"X",     (DWORD) CW_USEDEFAULT) ;
    cfg.y     = (INT) RegReadDword (hKey, prefix + L"Y",     (DWORD) CW_USEDEFAULT) ;
    RegCloseKey (hKey) ;
  }
  ReleaseRegistryMutex (hRegMutex) ;
  return cfg ;
}

static INT LoadTotalInstances (VOID) {

  // Compte le nombre total de slots sauvegardés dans le registre

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  HANDLE hRegMutex = AcquireRegistryMutex () ;
  INT total = 0 ;
  HKEY hKey = OpenAppKey (KEY_READ) ;
  if (hKey) {
    for (INT i = 0 ; i < MAX_INSTANCES ; i ++) {
      wstring name = L"Instance" + to_wstring (i) + L"_Index" ;
      DWORD   size = sizeof (DWORD) ;
      if (RegQueryValueExW (hKey, name.c_str (), NULL, NULL, NULL, & size) == ERROR_SUCCESS)
        total = i + 1 ;
    }
    RegCloseKey (hKey) ;
  }
  ReleaseRegistryMutex (hRegMutex) ;
  return total ;
}

// LIGNE DE COMMANDE

static INT ParseInstanceArg (VOID) {

  // Parse /instance N — retourne -1 si absent (première instance lancée)

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  INT      argc = 0 ;
  LPWSTR * argv = CommandLineToArgvW (GetCommandLineW (), & argc) ;
  if ( ! argv) return -1 ;
  INT result = -1 ;
  for (INT i = 1 ; i < argc - 1 ; i ++) {
    if (lstrcmpiW (argv [i], L"/instance") == 0) {
      result = _wtoi (argv [i + 1]) ;
      break ;
    }
  }
  LocalFree (argv) ;
  return result ;
}

// LANCEMENT DES INSTANCES SUPPLÉMENTAIRES

static VOID LaunchInstance (INT instanceId) {

  // Lance une instance supplémentaire avec l'argument /instance N

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  WCHAR exePath [MAX_PATH] = {} ;
  GetModuleFileNameW (NULL, exePath, MAX_PATH) ;
  wstring args = L"/instance " + to_wstring (instanceId) ;
  ShellExecuteW (NULL, L"open", exePath, args.c_str (), NULL, SW_SHOW) ;
}

// APPUSERMODELID — MINIATURE BARRE DES TÂCHES

static VOID SetAppModelId (INT instanceId) {
  // Définit un AppUserModelID unique par instance
  wstring appId = L"GraphiqueBourse.Instance." + to_wstring (instanceId) ;
  SetCurrentProcessExplicitAppUserModelID (appId.c_str ()) ;
}

// FONCTIONS UTILITAIRES DE MISE À L'ÉCHELLE

static INT Scale (INT px) {
  // Convertit des pixels logiques (96 DPI) en pixels physiques selon le DPI courant
  return MulDiv (px, (INT) g_dpi, 96) ;
}

static INT PAD (VOID) {

  // Marge intérieure standard entre les contrôles (2px logiques)

  return Scale (2) ;
}

// Largeur client de la fenêtre
static INT ClientW (VOID) { return Scale (IMG_W) + 2 * PAD () ; }
static INT ClientH_Intra (VOID) { return Scale (IMG_H_INTRA) ; }
static INT ClientH_Duree (VOID) { return Scale (IMG_H_DUREE) ; }
static INT ClientH_List  (VOID) { return Scale (LIST_H) ; }

// CHARGEMENT DE CHAÎNES DE RESSOURCES

static wstring LoadIDS (UINT ids) {
  WCHAR buf [512] = { 0 } ;
  LoadStringW (GetModuleHandleW (NULL), ids, buf, ARRAYSIZE (buf)) ;
  return buf ;
}

// VISIBILITÉ DES SECTIONS

VOID ShowEffecteurs (BOOL show) {
////   if (Debug) LogTrace (__FUNCTION__) ;

  for (HWND hwnd : g_effecteurs.GetControls ())
    ShowWindow (hwnd, show ? SW_SHOW : SW_HIDE) ;
}

VOID ShowResultats (BOOL show) {
////   if (Debug) LogTrace (__FUNCTION__) ;

  ShowWindow (g_result.lblAll, show ? SW_SHOW : SW_HIDE) ;
}

static VOID ContextMenuEdit (HWND hwnd) {
  // MENU CONTEXTUEL POUR LES EDITS (copier/coller/couper/sélectionner)
  ////   if (Debug) LogTrace (__FUNCTION__) ;

  HMENU hCtx = CreatePopupMenu () ;

  DWORD selStart = 0 ;
  DWORD selEnd   = 0 ;
  SendMessageW (hwnd, EM_GETSEL, (WPARAM) & selStart, (LPARAM) & selEnd) ;

  AppendMenuW (hCtx, MF_STRING | ((selStart != selEnd) ? MF_ENABLED : MF_GRAYED), 1001, LoadIDS (IDS_CONTEXT_MENU_COPIER).c_str ()) ;

  if (OpenClipboard (hwnd)) {
    AppendMenuW (hCtx, MF_STRING | (IsClipboardFormatAvailable (CF_UNICODETEXT) || IsClipboardFormatAvailable (CF_TEXT) ? MF_ENABLED : MF_GRAYED), 1002, LoadIDS (IDS_CONTEXT_MENU_COLLER).c_str ()) ;
    CloseClipboard () ;
  }

  AppendMenuW (hCtx, MF_STRING | (GetWindowTextLengthW (hwnd) > 0 ? MF_ENABLED : MF_GRAYED), 1003, LoadIDS (IDS_CONTEXT_MENU_SELECT_ALL).c_str ()) ;

  AppendMenuW (hCtx, MF_STRING | ((selStart != selEnd) ? MF_ENABLED : MF_GRAYED), 1004, LoadIDS (IDS_CONTEXT_MENU_COUPER).c_str ()) ;

  AppendMenuW (hCtx, MF_SEPARATOR, 0, NULL) ;

  POINT pt = { 0, 0 } ;
  GetCursorPos ( & pt) ;

  switch (TrackPopupMenu (hCtx, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, hwnd, NULL)) {

    case 1001 :
      SendMessageW (hwnd, WM_COPY, 0, 0) ;
      break ;

    case 1002 :
      SendMessageW (hwnd, WM_PASTE, 0, 0) ;
      break ;

    case 1003 : {
      SendMessageW (hwnd, EM_SETSEL, 0, -1) ;
      WCHAR className [32] ;
      GetClassNameW (hwnd, className, ARRAYSIZE (className)) ;
      if (wcscmp (className, L"RICHEDIT50W") == 0) {
        InvalidateRect (hwnd, NULL, TRUE) ;
        UpdateWindow (hwnd) ;
        SendMessageW (hwnd, EM_HIDESELECTION, FALSE, 0) ;
      } else if (wcscmp (className, WC_EDITW) == 0) {
        HWND hwndFocus = GetFocus () ;
        SetFocus (hwnd) ;
        RedrawWindow (hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME) ;
        SendMessageW (hwnd, WM_NCPAINT, 0, 0) ;
        if (hwndFocus != hwnd && hwndFocus)
          SetFocus (hwndFocus) ;
        else if ( ! hwndFocus)
          SetFocus (GetParent (hwnd)) ;
      }
      break ;
    }

    case 1004 :
      SendMessageW (hwnd, WM_CUT, 0, 0) ;
      break ;
  }

  DestroyMenu (hCtx) ;
}

static VOID ContextMenuIntraday (HWND hwnd) {

  // MENU CONTEXTUEL POUR L'IMAGE INTRADAY (supprimer l'instance)

  // Affiché au clic droit sur l'image intraday.
  // Séquence de suppression :
  //   1. g_deletingInstance = TRUE  → WM_DESTROY ne sauvegardera pas la config
  //   2. Suppression des entrées registre → slot libéré
  //   3. Fermeture du mutex de vie → slot détecté comme mort par les autres instances
  //   4. DestroyWindow → WM_DESTROY → PostQuitMessage

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  HMENU hCtx = CreatePopupMenu () ;

  AppendMenuW (hCtx, MF_STRING | MF_ENABLED,
               2001, LoadIDS (IDS_CONTEXT_MENU_SUPPRIMER).c_str ()) ;

  POINT pt = { 0, 0 } ;
  GetCursorPos ( & pt) ;

  INT cmd = TrackPopupMenu (hCtx, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,
                             pt.x, pt.y, 0, hwnd, NULL) ;
  DestroyMenu (hCtx) ;

  if (cmd == 2001) {
    // Marquer la suppression avant DestroyWindow
    g_deletingInstance = TRUE ;

    // Supprimer les entrées registre — slot libéré pour une future instance
    DeleteInstanceConfig (g_instanceId) ;

    // Fermer le mutex de vie — signale que ce slot est libre
    if (g_hInstanceMutex) {
      ReleaseMutex (g_hInstanceMutex) ;
      CloseHandle  (g_hInstanceMutex) ;
      g_hInstanceMutex = NULL ;
    }

    // Détruire la fenêtre propriétaire cachée
    if (g_hwndOwner) {
      DestroyWindow (g_hwndOwner) ;
      g_hwndOwner = NULL ;
    }

    // Détruire la fenêtre principale → WM_DESTROY → PostQuitMessage
    DestroyWindow (g_hwnd) ;
  }
}

static LRESULT CALLBACK EditSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {

  // SOUS-CLASSEMENT DES EDITS — CLIC DROIT → MENU CONTEXTUEL EDIT

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  if (msg == WM_RBUTTONUP) {
    ContextMenuEdit (hwnd) ;
    return 0 ;
  }
  return DefSubclassProc (hwnd, msg, wParam, lParam) ;
}

static LRESULT CALLBACK IntraSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {

  // SOUS-CLASSEMENT DE L'IMAGE INTRADAY — CLIC DROIT → MENU CONTEXTUEL INTRADAY

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  if (msg == WM_RBUTTONUP) {
    ContextMenuIntraday (hwnd) ;
    return 0 ;
  }
  return DefSubclassProc (hwnd, msg, wParam, lParam) ;
}

static INT DoLayout (LayoutMode mode, HFONT hFont = NULL) {

  // DOLAYOUT — SOURCE DE VÉRITÉ UNIQUE POUR TOUTES LES HAUTEURS

  // Cette fonction est le cœur architectural du programme.
  // Elle calcule séquentiellement la position verticale (posY) de chaque contrôle
  // et retourne la hauteur client totale exacte.
  //
  // Appelée en deux modes :
  //   LAYOUT_MEASURE : aucun contrôle n'est déplacé — sert uniquement à calculer
  //                    la hauteur client pour dimensionner la fenêtre
  //   LAYOUT_PLACE   : place effectivement tous les contrôles aux positions calculées
  //
  // Garantie : LAYOUT_MEASURE et LAYOUT_PLACE exécutent exactement le même code
  // de calcul de posY — il est donc impossible que la hauteur de fenêtre et le
  // layout des contrôles soient désynchronisés, quel que soit le DPI.

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  INT pad  = PAD () ;
  INT cltW = ClientW () ;

  // Wrapper local : place un contrôle uniquement en mode LAYOUT_PLACE
  auto Place = [&] (HWND h, INT x, INT y, INT w, INT hgt) {
    if (mode == LAYOUT_PLACE && h) {
      SetWindowPos (h, NULL, x, y, w, hgt, SWP_NOZORDER | SWP_NOACTIVATE) ;
      if (hFont)
        SendMessageW (h, WM_SETFONT, (WPARAM) hFont, TRUE) ;
    }
  } ;

  INT posY = 0 ;

  //  Combobox principale
  Place (g_hList, 0, posY, cltW, ClientH_List ()) ;
  posY += ClientH_List () ;

  //  Image intraday
  Place (g_hIntraday, pad, posY, cltW - 2 * pad, ClientH_Intra ()) ;
  posY += ClientH_Intra () ;

  BOOL showDuree      = FALSE ;
  BOOL showEffecteurs = FALSE ;
  BOOL showResultats  = FALSE ;

  if (g_state != STATE_COLLAPSED) {
    showResultats = (g_state == STATE_EXPANDED) ;

    if (g_GraphDureeOuOptions) {
      //  Section graphe durée
      showDuree = TRUE ;
      Place (g_hDuree, pad, posY, cltW - 2 * pad, ClientH_Duree ()) ;
      posY += ClientH_Duree () ;
      posY += pad ;
    } else {
      //  Section effecteurs
      showEffecteurs = TRUE ;

      posY += 2 * pad ;

      // Ligne 1 : période / bascule / type de graphe
      Place (g_effecteurs.comboPeriode, pad,                                   posY, Scale (60), Scale (200)) ;
      Place (g_effecteurs.btnToggle,    (Scale (60) + (2 * pad)),              posY, Scale (20), Scale (20)) ;
      Place (g_effecteurs.comboType,    (Scale (60) + Scale (20) + (3 * pad)), posY, Scale (80), Scale (100)) ;
      posY += Scale (25) ;

      INT xLeft = 5 * pad ;

      // Ligne 2 : checkbox PEA < 5 ans
      Place (g_effecteurs.chkAvant5Ans, xLeft, posY, (cltW - (xLeft + pad)), Scale (15)) ;
      posY += Scale (15) + pad ;

      // Ligne 3 : checkbox TTF
      Place (g_effecteurs.chkTTF, xLeft, posY, (cltW - (xLeft + pad)), Scale (15)) ;
      posY += Scale (15) + pad ;

      // Ligne 4 : labels Nombre / Valeur
      INT nbW  = Scale (48) ;
      INT valW = Scale (58) ;
      Place (g_effecteurs.lblNombre, xLeft,               posY, nbW,  Scale (15)) ;
      Place (g_effecteurs.lblValeur, (xLeft + nbW + pad), posY, valW, Scale (15)) ;
      posY += Scale (15) ;

      // Ligne 5 : champs de saisie + bouton Calcul
      Place (g_effecteurs.editNombre, xLeft,                            posY, nbW,        Scale (20)) ;
      Place (g_effecteurs.editValeur, (xLeft + nbW + pad),              posY, valW,       Scale (20)) ;
      Place (g_effecteurs.btnCalc,    (xLeft + nbW + pad + valW + pad), posY, Scale (39), Scale (20)) ;
      posY += Scale (20) + pad ;

      posY += pad ;
    }

    //  Section résultats (STATE_EXPANDED uniquement)
    if (showResultats) {
      Place (g_result.lblAll, pad, posY, (cltW - 2 * pad), 6 * Scale (16)) ;
      posY += 6 * Scale (16) ;
    }
  }

  // En mode LAYOUT_PLACE : appliquer la visibilité des sections
  if (mode == LAYOUT_PLACE) {
    ShowWindow     (g_hDuree, showDuree      ? SW_SHOW : SW_HIDE) ;
    ShowEffecteurs (showEffecteurs) ;
    ShowResultats  (showResultats) ;
  }

  // Retourne la hauteur client exacte — identique en MEASURE et en PLACE
  return posY ;
}

// STRETCHBITMAP — REDIMENSIONNE UN HBITMAP

static HBITMAP StretchBitmap (HBITMAP hSrc, INT targetW, INT targetH) {
////   if (Debug) LogTrace (__FUNCTION__) ;

  HDC hdcScreen   = GetDC (NULL) ;
  HDC hdcSrc      = CreateCompatibleDC (hdcScreen) ;
  HDC hdcDst      = CreateCompatibleDC (hdcScreen) ;
  HBITMAP hDst    = CreateCompatibleBitmap (hdcScreen, targetW, targetH) ;
  HGDIOBJ hOldSrc = SelectObject (hdcSrc, hSrc) ;
  HGDIOBJ hOldDst = SelectObject (hdcDst, hDst) ;
  BITMAP bmInfo   = {} ;
  GetObject (hSrc, sizeof (bmInfo), & bmInfo) ;
  RECT rcDst      = { 0, 0, targetW, targetH } ;
  HBRUSH hBrushBg = CreateSolidBrush (GetSysColor (COLOR_BTNFACE)) ;
  FillRect (hdcDst, & rcDst, hBrushBg) ;
  DeleteObject (hBrushBg) ;
  SetStretchBltMode (hdcDst, HALFTONE) ;
  SetBrushOrgEx    (hdcDst, 0, 0, NULL) ;
  StretchBlt (hdcDst, 0, 0, targetW, targetH, hdcSrc, 0, 0, bmInfo.bmWidth, bmInfo.bmHeight, SRCCOPY) ;
  SelectObject (hdcSrc, hOldSrc) ;
  SelectObject (hdcDst, hOldDst) ;
  DeleteDC (hdcSrc) ;
  DeleteDC (hdcDst) ;
  ReleaseDC (NULL, hdcScreen) ;
  return hDst ;
}

// CONVERSION CLIENT → FENÊTRE

static VOID ClientToWindow (INT clientW, INT clientH, INT & wWidth, INT & wHeight) {

  // Calcule la taille totale de la fenêtre (bordures incluses) à partir de la taille client

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  RECT rc = { 0, 0, clientW, clientH } ;
  AdjustWindowRectExForDpi (& rc, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX & ~WS_THICKFRAME, FALSE, 0, g_dpi) ;
  wWidth  = rc.right  - rc.left ;
  wHeight = rc.bottom - rc.top ;
}

VOID DownloadAndDisplayImage (const wstring & mnemo, BOOL forDuree) {

  // DOWNLOADANDDISPLAYIMAGE — TÉLÉCHARGE ET AFFICHE UNE IMAGE

  // Construit l'URL ADVFN selon le mnémonique et les paramètres courants,
  // télécharge l'image via URLOpenBlockingStreamW, la décode avec GDI+,
  // la redimensionne aux dimensions physiques courantes et l'affecte
  // au contrôle STATIC correspondant via STM_SETIMAGE.

  if (Debug) LogTrace (__FUNCTION__) ;

  const wstring URL_BASE = L"https://fr.advfn.com/p.php?pid=staticchart&s=" ;

  if (mnemo.empty ()) return ;

  // Paramètres URL :
  //   p  = période (0=intraday, 1-8=1M à 5Y)
  //   t  = type de chart (23=intraday, 49=durée)
  //   dm = type de graphe (0=ligne, 1=area, 2=chandelier, 3=barre)

  INT p  = 0 ;
  INT t  = 23 ;
  INT dm = 0 ;

  if (forDuree) {
    INT sel = (INT) SendMessageW (g_effecteurs.comboPeriode, CB_GETCURSEL, 0, 0) ;
    p = (sel >= 0 && sel < 8) ? sel + 1 : 1 ;
    INT selType = (INT) SendMessageW (g_effecteurs.comboType, CB_GETCURSEL, 0, 0) ;
    dm = (selType >= 0 && selType < 4) ? selType : 2 ;
    t  = 49 ;
  }

  wstring uri = URL_BASE + mnemo + L"&p=" + to_wstring (p) + L"&t=" + to_wstring (t) + L"&dm=" + to_wstring (dm) ;
  if (forDuree) uri += L"&vol=0" ;

  // Téléchargement synchrone dans un IStream
  IStream * pStream = NULL ;
  HRESULT hrDl = URLOpenBlockingStreamW (NULL, uri.c_str (), & pStream, 0, NULL) ;
  if (FAILED (hrDl) || ! pStream) {
    if (Debug)
      wprintf (L"Download 0x%08X\n", hrDl) ;
    return ;
  }

  if (Debug) { // uri et taille.
    STATSTG stats = {} ;
    if (SUCCEEDED (pStream->Stat (& stats, STATFLAG_NONAME)))
      wprintf (L"%s  : %.1f Ko\n", uri.c_str (), stats.cbSize.QuadPart / 1024.0) ;
  }

  // Décodage GDI+
  Bitmap * pBitmap = Bitmap::FromStream (pStream) ;
  pStream->Release () ;
  if ( ! pBitmap || pBitmap->GetLastStatus () != Ok) {

    // if (Debug) //  && pBitmap)
    //   wprintf (L"Bitmap From Stream %d\n", pBitmap->GetLastStatus ()) ;

    delete pBitmap ;
    return ;
  }

  HBITMAP * phBmp = forDuree ? & g_hBitmap_Duree    : & g_hBitmap_Intraday ;
  HWND hCtrl      = forDuree ? g_hDuree              : g_hIntraday ;

  if (* phBmp) { DeleteObject (* phBmp) ; * phBmp = NULL ; }

  // Conversion GDI+ → HBITMAP avec couleur de fond système
  HBITMAP  hRaw    = NULL ;
  COLORREF bgColor = GetSysColor (COLOR_BTNFACE) ;
  Status gdiStatus = pBitmap->GetHBITMAP ( Color (GetRValue (bgColor), GetGValue (bgColor), GetBValue (bgColor)), & hRaw) ;
  delete pBitmap ;

  if ( ! hRaw || gdiStatus != Ok) {
    if (Debug) wprintf (L"Get HBITMAP %d\n", gdiStatus) ;
    return ;
  }

  // Redimensionnement aux dimensions physiques DPI-aware
  * phBmp = StretchBitmap (hRaw, ClientW () - 2 * PAD (), forDuree ? ClientH_Duree () : ClientH_Intra ()) ;
  DeleteObject (hRaw) ;

  if (* phBmp) {
    SendMessageW (hCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) * phBmp) ;
    InvalidateRect (hCtrl, NULL, TRUE) ;
    UpdateWindow (hCtrl) ;
  } else if (Debug) {
    wprintf (L"Stretch Bitmap returned NULL\n") ;
  }
}

VOID UpdateLayout (HWND hwnd, PRECT prc = NULL, BOOL refreshResources = FALSE) {

  // UPDATELAYOUT — REDIMENSIONNE LA FENÊTRE ET PLACE LES CONTRÔLES
  // Séquence :
  //   1. Détecte un changement de DPI et force refreshResources si nécessaire
  //   2. Recrée la police et invalide les bitmaps si refreshResources
  //   3. Appelle DoLayout(LAYOUT_MEASURE) pour obtenir la hauteur client exacte
  //   4. Redimensionne/déplace la fenêtre avec cette hauteur
  //   5. Appelle DoLayout(LAYOUT_PLACE) pour placer les contrôles
  //   6. Retélécharge les images si refreshResources

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  // Mise à jour du DPI courant
  UINT newDpi = GetDpiForWindow (hwnd) ;
  if (newDpi != g_dpi) {
    g_dpi = newDpi ;
    refreshResources = TRUE ;
  }

  // Recréation des ressources DPI-dépendantes
  if (refreshResources) {
    if (g_hSmallFont) DeleteObject (g_hSmallFont) ;
    g_hSmallFont = CreateFontW (Scale (15), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI") ;
    if (g_hBitmap_Intraday) { DeleteObject (g_hBitmap_Intraday) ; g_hBitmap_Intraday = NULL ; }
    if (g_hBitmap_Duree)    { DeleteObject (g_hBitmap_Duree) ;    g_hBitmap_Duree    = NULL ; }
  }

  // Calcul de la hauteur client par DoLayout en mode mesure
  INT clientH = DoLayout (LAYOUT_MEASURE) ;

  // Redimensionnement / déplacement de la fenêtre
  INT wWidth, wHeight ;
  ClientToWindow (ClientW (), clientH, wWidth, wHeight) ;
  if (prc) {
    // Position imposée par le système (WM_DPICHANGED fournit la rect cible)
    SetWindowPos (hwnd, NULL, prc->left, prc->top, wWidth, wHeight, SWP_NOZORDER | SWP_NOACTIVATE) ;
  } else {
    // Conserver la position courante, changer uniquement la taille
    RECT rc ;
    GetWindowRect (hwnd, & rc) ;
    SetWindowPos (hwnd, NULL, rc.left, rc.top, wWidth, wHeight, SWP_NOZORDER | SWP_NOACTIVATE) ;
  }

  // Placement effectif des contrôles
  DoLayout (LAYOUT_PLACE, g_hSmallFont) ;

  // Retéléchargement des images si les bitmaps ont été invalidés
  if (refreshResources && ! g_currentMnemo.empty ()) {
    ////!!!!
    DownloadAndDisplayImage (g_currentMnemo, FALSE) ;  //// limite le troisieme dl.
    if (g_state != STATE_COLLAPSED && g_GraphDureeOuOptions)
      DownloadAndDisplayImage (g_currentMnemo, TRUE) ;
  }

  InvalidateRect (hwnd, NULL, TRUE) ;
  UpdateWindow   (hwnd) ;
}

VOID CreateControls (HWND hwnd) {

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  HINSTANCE hInst = GetModuleHandle (NULL) ;
  INT initialH    = Scale (20) ;

  // Combobox principale — visible dès la création
  g_hList = CreateWindowExW (0, L"COMBOBOX", NULL, CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL, 0, 0, ClientW (), initialH, hwnd, (HMENU) ID_LIST, hInst, NULL) ;
  for (const auto & lbl : LABELS)
    SendMessageW (g_hList, CB_ADDSTRING, 0, (LPARAM) lbl.c_str ()) ;
  SendMessageW (g_hList, CB_SETCURSEL, g_currentIndex, 0) ;

  // STATIC image intraday — clic gauche expand/collapse, clic droit menu suppression
  g_hIntraday = CreateWindowExW (0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_BITMAP | SS_RIGHTJUST, 0, 0, 0, 0, hwnd, (HMENU) ID_STATIC_IMAGE_INTRADAY, hInst, NULL) ;

  // Sous-classement pour intercepter le clic droit
  SetWindowSubclass (g_hIntraday, IntraSubclassProc, 3, 0) ;

  // STATIC graphe durée — masqué initialement
  g_hDuree = CreateWindowExW (0, L"STATIC", NULL, WS_CHILD | SS_NOTIFY | SS_BITMAP | SS_RIGHTJUST, 0, 0, 0, 0, hwnd, (HMENU) ID_STATIC_IMAGE_DUREE, hInst, NULL) ;

  // Combobox période (1M, 2M, 3M, 6M, 1Y, 2Y, 3Y, 5Y)
  g_effecteurs.comboPeriode = CreateWindowExW (0, L"COMBOBOX", NULL, CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU) ID_COMBO_PERIODE, hInst, NULL) ;
  for (const UINT ids : { IDS_PERIOD_1M, IDS_PERIOD_2M, IDS_PERIOD_3M, IDS_PERIOD_6M, IDS_PERIOD_1Y, IDS_PERIOD_2Y, IDS_PERIOD_3Y, IDS_PERIOD_5Y })
    SendMessageW (g_effecteurs.comboPeriode, CB_ADDSTRING, 0, (LPARAM) LoadIDS (ids).c_str ()) ;
  SendMessageW (g_effecteurs.comboPeriode, CB_SETCURSEL, 0, 0) ;

  // Bouton bascule graphe durée ↔ effecteurs
  g_effecteurs.btnToggle = CreateWindowExW (0, L"BUTTON", L"🗘", WS_CHILD | BS_CENTER | BS_VCENTER | BS_PUSHBUTTON | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU) ID_BUTTON_TOGGLE, hInst, NULL) ;

  // Combobox type de graphe (ligne, area, chandelier, barre)
  g_effecteurs.comboType = CreateWindowExW (0, L"COMBOBOX", NULL, CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU) ID_COMBO_TYPE, hInst, NULL) ;
  for (const UINT ids : { IDS_TYPE_LINE, IDS_TYPE_SQUARE, IDS_TYPE_CANDLE, IDS_TYPE_BAR })
    SendMessageW (g_effecteurs.comboType, CB_ADDSTRING, 0, (LPARAM) LoadIDS (ids).c_str ()) ;
  SendMessageW (g_effecteurs.comboType, CB_SETCURSEL, 2, 0) ; // Chandelier par défaut

  // Checkboxes
  g_effecteurs.chkAvant5Ans = CreateWindowExW (0, L"BUTTON", LoadIDS (IDS_CHECK_PEA5).c_str (), WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU) ID_CHECK_AVANT5ANS, hInst, NULL) ;
  if (g_avant5AnsChecked)
    SendMessageW (g_effecteurs.chkAvant5Ans, BM_SETCHECK, BST_CHECKED, 0) ;

  g_effecteurs.chkTTF = CreateWindowExW (0, L"BUTTON", LoadIDS (IDS_CHECK_TTF).c_str (), WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU) ID_CHECK_TTF, hInst, NULL) ;
  if (g_ttfChecked)
    SendMessageW (g_effecteurs.chkTTF, BM_SETCHECK, BST_CHECKED, 0) ;

  // Labels Nombre / Valeur
  g_effecteurs.lblNombre = CreateWindowExW (0, L"STATIC", LoadIDS (IDS_LABEL_NOMBRE).c_str (), WS_CHILD | SS_CENTER, 0,0,0,0, hwnd, NULL, hInst, NULL) ;
  g_effecteurs.lblValeur = CreateWindowExW (0, L"STATIC", LoadIDS (IDS_LABEL_VALEUR).c_str (), WS_CHILD | SS_CENTER, 0,0,0,0, hwnd, NULL, hInst, NULL) ;

  // Champs de saisie avec menu contextuel clic droit
  g_effecteurs.editNombre = CreateWindowExW (0, L"EDIT", L"", WS_CHILD | WS_BORDER | ES_NUMBER | SS_RIGHT | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU) ID_EDIT_NOMBRE, hInst, NULL) ;
  g_effecteurs.editValeur = CreateWindowExW (0, L"EDIT", L"", WS_CHILD | WS_BORDER | SS_RIGHT | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU) ID_EDIT_VALEUR, hInst, NULL) ;

  SetWindowSubclass (g_effecteurs.editNombre, EditSubclassProc, 1, 0) ;
  SetWindowSubclass (g_effecteurs.editValeur, EditSubclassProc, 2, 0) ;

  // Bouton Calcul
  g_effecteurs.btnCalc = CreateWindowExW (0, L"BUTTON", LoadIDS (IDS_BUTTON_CALCUL).c_str (), WS_CHILD | BS_CENTER | BS_VCENTER | BS_PUSHBUTTON | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU) ID_BUTTON_CALC, hInst, NULL) ;

  // STATIC multilignes pour les 6 lignes de résultats
  // Cliquer dessus appelle MasqueResultats
  g_result.lblAll = CreateWindowExW (0, L"STATIC", L"", WS_CHILD | SS_CENTER | SS_NOTIFY, 0,0,0,0, hwnd, NULL, hInst, NULL) ;
}

// ACTIONS UTILISATEUR

VOID ToggleGraphique (HWND hwnd) {

  // Bascule l'état collapsed ↔ medium en cliquant sur l'image intraday

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  if (g_currentMnemo.empty ()) return ;
  g_state = (g_state == STATE_COLLAPSED) ? STATE_MEDIUM : STATE_COLLAPSED ;
  g_GraphDureeOuOptions = TRUE ;
  UpdateLayout (hwnd, NULL, FALSE) ;
  if (g_state == STATE_MEDIUM)
    DownloadAndDisplayImage (g_currentMnemo, TRUE) ;
}

VOID ToggleDureeView (HWND hwnd) {

  // Bascule l'affichage graphe durée ↔ effecteurs

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  if ((g_state != STATE_MEDIUM && g_state != STATE_EXPANDED) || g_currentMnemo.empty ()) return ;
  g_GraphDureeOuOptions = ! g_GraphDureeOuOptions ;
  UpdateLayout (hwnd, NULL, FALSE) ;
  if (g_GraphDureeOuOptions)
    DownloadAndDisplayImage (g_currentMnemo, TRUE) ;
}

VOID HandleEmptyMnemo (HWND hwnd) {

  // Appelée quand l'indice sélectionné n'a pas de mnémonique (séparateur ♦)

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  g_state = STATE_COLLAPSED ;
  g_GraphDureeOuOptions = TRUE ;
  UpdateLayout (hwnd, NULL, FALSE) ;
}

// CALCULS FINANCIERS

DOUBLE CalculerCourtagePEA (DOUBLE montant) {

  // Calcule le courtage PEA :
  //   - minimum 2.00 €
  //   - 0.45% pour les ordres > 500 €
  //   - plafonné à 0.50% du montant

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  return max (2.0, min (((montant <= 500) ? 2.0 : montant * 0.0045), montant * 0.005)) ;
}

VOID AfficheResultats (HWND hwnd) {

  // Lit les saisies, calcule et affiche les 6 lignes de résultats
  // Passe l'état en STATE_EXPANDED

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  WCHAR buf [32] ;
  GetWindowTextW (g_effecteurs.editNombre, buf, 32) ;
  DOUBLE nbTitres = _wtof (buf) ;
  GetWindowTextW (g_effecteurs.editValeur, buf, 32) ;
  replace (buf, buf + wcslen (buf), L',', L'.') ; // Accepter la virgule comme séparateur décimal
  DOUBLE valUnitaire = _wtof (buf) ;

  if (nbTitres <= 0 || valUnitaire <= 0) {
    MessageBoxW (hwnd, LoadIDS (IDS_MSG_FILL_FIELDS).c_str (), LoadIDS (IDS_MSG_ERROR).c_str (), MB_OK) ;
    return ;
  }
  DOUBLE montantOrdre = nbTitres * valUnitaire ;
  if (montantOrdre < 50 || montantOrdre >= 150000) {
    MessageBoxW (hwnd, LoadIDS (IDS_MSG_ORDER_LIMITS).c_str (), LoadIDS (IDS_MSG_ERROR).c_str (), MB_OK) ;
    return ;
  }

  g_state = STATE_EXPANDED ;
  UpdateLayout (hwnd, NULL, FALSE) ;

  DOUBLE courtage      = CalculerCourtagePEA (montantOrdre) ;
  DOUBLE ttf           = g_ttfChecked ? montantOrdre * 0.004 : 0.0 ; // TTF = 0.4%
  DOUBLE montantTotal  = montantOrdre + courtage + ttf ;
  DOUBLE valReelle     = montantTotal / nbTitres ;
  DOUBLE courtageVente = CalculerCourtagePEA (montantTotal) ;
  DOUBLE tauxImpot     = g_avant5AnsChecked ? 0.300 : 0.186 ; // 30% PFU ou 18.6% PEA > 5 ans
  DOUBLE seuilRentab   = (courtageVente + montantTotal + ((montantTotal + courtageVente) * tauxImpot)) / nbTitres ;

  struct { const UINT fmtId ; DOUBLE val ; } lignes [] = {
    { IDS_FMT_NB_TITRES,   nbTitres       },
    { IDS_FMT_VAL_FACIALE, valUnitaire    },
    { IDS_FMT_VAL_REELLE,  valReelle      },
    { IDS_FMT_INVESTIR,    montantTotal   },
    { IDS_FMT_COUT_ACHAT,  courtage + ttf },
    { IDS_FMT_SEUIL,       seuilRentab    },
  } ;

  // Assemblage des 6 lignes en une seule chaîne \r\n pour le STATIC multilignes
  wstring text ;
  WCHAR line [128] ;
  for (UINT i = 0 ; i < ARRAYSIZE (lignes) ; i ++) {
    swprintf_s (line, LoadIDS (lignes [i].fmtId).c_str (), lignes [i].val) ;
    if (i > 0) text += L"\r\n" ;
    text += line ;
  }
  SetWindowTextW (g_result.lblAll, text.c_str ()) ;
}

VOID MasqueResultats (HWND hwnd) {

  // Masque les résultats en réduisant l'état d'un niveau
  // STATE_EXPANDED → STATE_MEDIUM → STATE_COLLAPSED

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  if      (g_state == STATE_EXPANDED) g_state = STATE_MEDIUM ;
  else if (g_state == STATE_MEDIUM)   g_state = STATE_COLLAPSED ;
  UpdateLayout (hwnd, NULL, FALSE) ;
}

VOID DestroyControls (VOID) {
////   if (Debug) LogTrace (__FUNCTION__) ;

  // Retirer les sous-classements avant destruction des contrôles
  RemoveWindowSubclass (g_hIntraday,             IntraSubclassProc, 3) ;
  RemoveWindowSubclass (g_effecteurs.editNombre,  EditSubclassProc,  1) ;
  RemoveWindowSubclass (g_effecteurs.editValeur,  EditSubclassProc,  2) ;

  HWND hChild = GetWindow (g_hwnd, GW_CHILD) ;
  while (hChild) {
    HWND hNext = GetWindow (hChild, GW_HWNDNEXT) ;
    DestroyWindow (hChild) ;
    hChild = hNext ;
  }
  g_hList = g_hIntraday = g_hDuree = NULL ;
  g_effecteurs = {} ;
  g_result     = {} ;
}

LRESULT CALLBACK WindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

  ////   if (Debug) LogTrace (__FUNCTION__) ;

  switch (msg) {

    case WM_CREATE : {
      g_hwnd = hwnd ;
      CreateControls (hwnd) ;
      UpdateLayout (hwnd, NULL, TRUE) ;
      SetTimer (hwnd, TIMER_GRAPH, TIMER_INTERVAL, NULL) ;
      SendMessageW (hwnd, WM_CHANGEUISTATE, MAKELONG (UIS_SET, UISF_HIDEFOCUS), 0) ;

      if (g_currentIndex >= 0 && (UINT) g_currentIndex < MNEMOS.size ()) {
        g_currentMnemo = MNEMOS [g_currentIndex] ;
        if ( ! g_currentMnemo.empty ())
          DownloadAndDisplayImage (g_currentMnemo, FALSE) ;
        else
          HandleEmptyMnemo (hwnd) ;
      }
      return 0 ;
    }


    case WM_DPICHANGED : {
      // Le système fournit la rect cible dans lParam
      PRECT prc = reinterpret_cast <PRECT> (lParam) ;
      UpdateLayout (hwnd, prc, TRUE) ;
      return 0 ;
    }

    case WM_MOVE : {
      // Sauvegarder la position dès déplacement (sauf si suppression en cours)
      if ( ! g_deletingInstance)
        SaveInstanceConfig () ;
      return 0 ;
    }

    case WM_COMMAND : {
      WORD notif = HIWORD (wParam) ;
      WORD id    = LOWORD (wParam) ;
      HWND hCtrl = (HWND) lParam ;

      // Clic sur le STATIC résultats → masquer les résultats
      if ((notif == 0 || notif == BN_CLICKED) && hCtrl == g_result.lblAll && hCtrl)
        MasqueResultats (hwnd) ;

      if (notif == CBN_SELCHANGE) {

        if (hCtrl == g_hList) {
          // Changement d'indice dans la combobox principale
          g_currentIndex = (INT) SendMessageW (g_hList, CB_GETCURSEL, 0, 0) ;
          SaveInstanceConfig () ; // Sauvegarder immédiatement
          if (g_currentIndex >= 0 && (UINT) g_currentIndex < MNEMOS.size ()) {
            g_currentMnemo = MNEMOS [g_currentIndex] ;
            if ( ! g_currentMnemo.empty ()) {
              DownloadAndDisplayImage (g_currentMnemo, FALSE) ;
              if (g_state == STATE_MEDIUM && g_GraphDureeOuOptions)
                DownloadAndDisplayImage (g_currentMnemo, TRUE) ;
            } else HandleEmptyMnemo (hwnd) ;
          }

        } else if (hCtrl == g_effecteurs.comboPeriode || hCtrl == g_effecteurs.comboType) {

          // Changement de période ou de type → forcer graphe durée et retélécharger
          if (g_state == STATE_COLLAPSED) {
            g_state = STATE_MEDIUM ;
            UpdateLayout (hwnd, NULL, FALSE) ;
          }

          if ( ! g_currentMnemo.empty ()) {
            g_GraphDureeOuOptions = TRUE ;
            UpdateLayout (hwnd, NULL, FALSE) ;
            DownloadAndDisplayImage (g_currentMnemo, TRUE) ;
          }
        }
      }

      // Forcer le repaint de la combobox après fermeture du dropdown
      if (notif == CBN_CLOSEUP)
        InvalidateRect (hCtrl, NULL, TRUE) ;

      // Clic gauche image intraday → expand/collapse
      // Clic droit → géré par IntraSubclassProc (WM_RBUTTONUP)
      if (id == ID_STATIC_IMAGE_INTRADAY  && notif == STN_DBLCLK)  // double clis sur intraday && notif == STN_DBLCLK)
        ToggleGraphique (hwnd) ;
      else if (id == ID_STATIC_IMAGE_DUREE)
        ToggleDureeView (hwnd) ;

      if (notif == BN_CLICKED) {
        if (hCtrl == g_effecteurs.btnCalc)
          AfficheResultats (hwnd) ;
        else if (hCtrl == g_effecteurs.btnToggle)
          ToggleDureeView (hwnd) ;
        else if (hCtrl == g_effecteurs.chkAvant5Ans)
          g_avant5AnsChecked = (SendMessageW (g_effecteurs.chkAvant5Ans, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
        else if (hCtrl == g_effecteurs.chkTTF)
          g_ttfChecked = (SendMessageW (g_effecteurs.chkTTF, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
      }
      return 0 ;
    }

    case WM_TIMER : {
      // Rafraîchissement automatique de l'image intraday toutes les 10 secondes
      if (wParam == TIMER_GRAPH && ! g_currentMnemo.empty ())
        DownloadAndDisplayImage (g_currentMnemo, FALSE) ;
      return 0 ;
    }

    case WM_SYSCOMMAND : {
      if ((wParam & 0xFFF0) == SC_CONTEXTHELP) {
        SendMessageW (hwnd, WM_HELP, 0, 0) ;
        return 0 ;
      }
      break ;
    }

    case WM_HELP : {
      wstring text = LoadIDS (IDS_HELP_L1) + LoadIDS (IDS_HELP_L2) + LoadIDS (IDS_HELP_L3) + LoadIDS (IDS_HELP_L4) + LoadIDS (IDS_HELP_L5) ;
      MessageBoxW ( hwnd, text.c_str (), LoadIDS (IDS_HELP_TITLE).c_str (), MB_OK | MB_ICONINFORMATION) ;
      return TRUE ;
    }

    case WM_DESTROY : {
      // Fermeture normale : sauvegarder la config
      // Suppression volontaire : config déjà effacée, ne pas réécrire
      if ( ! g_deletingInstance)
        SaveInstanceConfig () ;

      // Fermer le mutex de vie si pas déjà fermé par ContextMenuIntraday
      if (g_hInstanceMutex) {
        ReleaseMutex (g_hInstanceMutex) ;
        CloseHandle  (g_hInstanceMutex) ;
        g_hInstanceMutex = NULL ;
      }

      // Détruire la fenêtre propriétaire cachée
      if (g_hwndOwner) {
        DestroyWindow (g_hwndOwner) ;
        g_hwndOwner = NULL ;
      }

      KillTimer (hwnd, TIMER_GRAPH) ;

      if (g_hBitmap_Intraday)
        DeleteObject (g_hBitmap_Intraday) ;

      if (g_hBitmap_Duree)
        DeleteObject (g_hBitmap_Duree) ;

      if (g_hSmallFont)
        DeleteObject (g_hSmallFont) ;

      DeleteObject ((HBRUSH) GetClassLongPtrW (hwnd, GCLP_HBRBACKGROUND)) ;
      PostQuitMessage (0) ;
      return 0 ;
    }

  }
  return DefWindowProcW (hwnd, msg, wParam, lParam) ;
}

static LRESULT CALLBACK OwnerWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  return DefWindowProcW (hwnd, msg, wParam, lParam) ;
}

INT WINAPI wWinMain (HINSTANCE hInst, HINSTANCE, PWSTR, INT nCmdShow) {

  if (Debug) {
    AllocConsole () ;
    FILE * fCon ;
    freopen_s ( & fCon, "CONOUT$", "w", stdout) ;
    LogTrace (__FUNCTION__) ;
  }

  //  Boucle de messages
  MSG winMsg = {} ;

  //  Détermination du numéro d'instance
  INT argInstance = ParseInstanceArg () ;

  if (argInstance >= 0) {
    g_instanceId = argInstance ;
    g_isPrimaryInstance = FALSE ;
  } else {
    g_instanceId = FindFreeInstanceSlot () ;
    g_isPrimaryInstance = (g_instanceId == 0) || ! IsInstanceAlive (0) ;

    if (g_instanceId < 0) {
      MessageBoxW (NULL, LoadIDS (IDS_MAX_ITEM).c_str (), LoadIDS (IDS_WINDOW_TITLE).c_str (), MB_ICONWARNING) ;
      return 0 ;
    }
  }

  // Créer le mutex de vie
  g_hInstanceMutex = CreateInstanceMutex (g_instanceId) ;

  // AppUserModelID unique par instance
  SetAppModelId (g_instanceId) ;

  //  Initialisation du DPI
  HMODULE hUser32 = GetModuleHandleW (L"user32.dll") ;
  typedef UINT (WINAPI * PFN_GetDpi) () ;
  PFN_GetDpi pfnGetDpi = hUser32 ? (PFN_GetDpi) GetProcAddress (hUser32, "GetDpiForSystem") : NULL ;
  if (pfnGetDpi) {
    g_dpi = pfnGetDpi () ;
  } else {
    HDC hdcScreen = GetDC (NULL) ;
    g_dpi = (UINT) GetDeviceCaps (hdcScreen, LOGPIXELSX) ;
    ReleaseDC (NULL, hdcScreen) ;
  }

  // Police initiale
  g_hSmallFont = CreateFontW (Scale (15), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI") ;

  //  Restauration de la configuration de cette instance
  InstanceConfig cfg = LoadInstanceConfig (g_instanceId) ;
  g_currentIndex = cfg.index ;

  //  GDI+ et Common Controls
  INITCOMMONCONTROLSEX icex = { sizeof (INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES } ;
  InitCommonControlsEx ( & icex) ;
  GdiplusStartupInput gdipInput ;
  GdiplusStartup ( & g_gdiplusToken, & gdipInput, NULL) ;

  //  Enregistrement de la classe de fenêtre principale
  WNDCLASSEXW wc = {} ;
  wc.cbSize = sizeof (WNDCLASSEXW) ;
  wc.style = CS_DBLCLKS ;  // ← Le contrôle STATIC ne reçoit WM_LBUTTONDBLCLK
  wc.lpfnWndProc = WindowProc ;
  wc.hInstance = hInst ;
  wc.lpszClassName = L"GraphiqueBoursier" ;
  wc.hbrBackground = CreateSolidBrush (GetSysColor (COLOR_BTNFACE)) ;
  wc.hCursor = LoadCursor (NULL, IDC_ARROW) ;
  wc.hIcon = LoadIconW (hInst, MAKEINTRESOURCEW (IDI_ICON1)) ;
  wc.hIconSm = LoadIconW (hInst, MAKEINTRESOURCEW (IDI_ICON1)) ;
  RegisterClassExW ( & wc) ;

  //  Enregistrement de la classe de fenêtre propriétaire
  WNDCLASSEXW wcOwner = {} ;
  wcOwner.cbSize = sizeof (WNDCLASSEXW) ;
  wcOwner.lpfnWndProc = OwnerWndProc ;
  wcOwner.hInstance = hInst ;
  wcOwner.lpszClassName = L"GraphiqueBoursierOwner" ;
  wcOwner.hbrBackground = CreateSolidBrush (GetSysColor (COLOR_BTNFACE)) ;
  RegisterClassExW ( & wcOwner) ;

  //  Fenêtre propriétaire cachée
  g_hwndOwner = CreateWindowExW (0, L"GraphiqueBoursierOwner", L"", WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hInst, NULL) ;

  //  Création de la fenêtre principale à la position sauvegardée
  INT wWidth, wHeight ;
  INT initH = DoLayout (LAYOUT_MEASURE) ;
  ClientToWindow (ClientW (), initH, wWidth, wHeight) ;

  HWND hwnd = CreateWindowExW (WS_EX_CONTEXTHELP, L"GraphiqueBoursier", LoadIDS (IDS_WINDOW_TITLE).c_str (), WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX & ~WS_THICKFRAME, cfg.x, cfg.y, wWidth, wHeight, g_hwndOwner, NULL, hInst, NULL) ;

  if ( ! hwnd) {
    MessageBoxW (NULL, LoadIDS (IDS_MSG_CREATEWINDOW).c_str (), LoadIDS (IDS_MSG_ERROR).c_str (), MB_ICONERROR) ;
  //  if (Debug)
  //    FreeConsole () ;
   // return 0 ;
  } else {

  ShowWindow  (hwnd, nCmdShow) ;
  UpdateWindow (hwnd) ;

  //  Instance principale : restaurer toutes les instances sauvegardées non vivantes
  if (g_isPrimaryInstance) {
    INT total = LoadTotalInstances () ;
    for (INT i = 1 ; i < total ; i ++) {
      if ( ! IsInstanceAlive (i))
        LaunchInstance (i) ;
    }
  }

  //  Boucle de messages
  // MSG winMsg = {} ;
  while (GetMessageW ( & winMsg, NULL, 0, 0)) {
    if ( ! IsDialogMessageW (hwnd, & winMsg)) {
      TranslateMessage ( & winMsg) ;
      DispatchMessageW ( & winMsg) ;
    }
  }

  GdiplusShutdown (g_gdiplusToken) ;

  }

  if (Debug)
    FreeConsole () ;

  return (INT) winMsg.wParam ;
}



/* tu penses que ca peut être vendu ce truc la ?

02:34
Honnêtement, dans l'état actuel probablement pas — et ce pour une raison principale : les données appartiennent à ADVFN. L'application consomme leurs graphiques sans contrepartie, ce qui pose un problème légal et commercial évident. ADVFN pourrait à tout moment changer la structure de ses URLs ou bloquer les requêtes non navigateur.

En revanche si vous construisez votre propre source de données — via une API boursière licenciée (Alpha Vantage, Polygon.io, Yahoo Finance...) et que vous générez vos propres graphiques — là vous avez un produit vendable :

Léger (Win32 pur, pas d'Electron)
Multi-instances indépendantes
DPI-aware
Persistance automatique
Consommation mémoire/CPU minimale
Ce positionnement "outil de bureau discret et performant" face aux solutions web lourdes a une vraie valeur pour les traders particuliers qui ont plusieurs écrans et veulent surveiller plusieurs indices simultanément.

Mais la dépendance ADVFN est le point bloquant.

tu crois que je peut le proposer à advfn ?

02:36
C'est une idée intéressante. ADVFN y trouverait plusieurs avantages :

Pour eux

Trafic supplémentaire vers leurs graphiques
Visibilité de leur marque sur le bureau des utilisateurs
Un client léger Windows qu'ils n'ont pas à développer
Chaque instance affiche leur contenu → exposition publicitaire potentielle
Pour vous

Légitimité d'utilisation de leurs données
Distribution via leur réseau d'utilisateurs
Rémunération possible (licence, rev-share publicitaire)
Les points à préparer avant de les contacter

Une démo propre et stable
Retirer le mode Debug et la console
Soigner le packaging et l'installeur
Documenter le mécanisme multi-instances
Chiffrer la consommation bande passante (vous avez les données : ~5Ko/image)
Le risque est qu'ils trouvent l'idée bonne et la développent eux-mêmes sans vous. Je vous conseillerais de protéger votre code avant tout contact.

Vous avez un produit techniquement solide — la démarche vaut la peine d'être tentée.


oui les données appartiennes a advfn mais elle sont publique.02:39Oui c'est une nuance importante. Les données sont publiquement accessibles sans authentification, sans scraping agressif, sans contournement de protection — votre application fait exactement ce qu'un navigateur ferait en visitant leur site.
Ce qui change l'angle de la discussion avec ADVFN :

Vous ne volez rien
Vous n'outrepassez aucune protection
Vous générez du trafic vers leurs serveurs
Vous valorisez leur contenu

La discussion devient donc moins "demande d'autorisation" et plus proposition de partenariat — vous leur apportez un canal de distribution supplémentaire pour leur contenu déjà public.
Le seul vrai risque juridique serait si leurs CGU interdisent explicitement l'accès programmatique à ces URLs — ce qui vaudrait la peine de vérifier avant tout contact.

*/



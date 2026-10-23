#pragma once
#include <windows.h>

enum AppLangId {
    LANG_ID_TR = 0,
    LANG_ID_EN,
    LANG_ID_DE,
    LANG_ID_FR,
    LANG_ID_ES,
    LANG_ID_PT,
    LANG_ID_IT,
    LANG_ID_RU,
    LANG_ID_JA,
    LANG_ID_KO,
    LANG_ID_ZH,
    LANG_ID_AR,
    LANG_ID_NL,
    LANG_ID_PL,
    LANG_ID_HI,
    LANG_ID_ID,
    LANG_ID_TH,
    LANG_ID_VI,
    LANG_ID_UK,
    LANG_ID_COUNT
};

struct UiStrings {
    const wchar_t* code;
    const wchar_t* menuLabel;
    const wchar_t* shortCode;

    // Header
    const wchar_t* statusActiveFmt;   // "●  Aktif • Kademe %d"
    const wchar_t* statusReadyFmt;    // "○  Hazır • %.1f MB"

    // Card 1: Scope & Stage
    const wchar_t* secScope;          // "ÇALIŞMA KAPSAMI"
    const wchar_t* scope0Title;       // "Sadece Liste"
    const wchar_t* scope0Sub;         // "Seçili hedefleri tünelle"
    const wchar_t* scope1Title;       // "Hariç Tutulanlar"
    const wchar_t* scope1Sub;         // "Liste dışındakileri aç"
    const wchar_t* scope2Title;       // "Tüm Sistem"
    const wchar_t* scope2Sub;         // "Her şeyi koşulsuz aç"

    const wchar_t* secStage;          // "DPI KADEMESİ"
    const wchar_t* stageHint;         // "Kutucuğa bastığın an geçiş yapar"
    const wchar_t* stageLabels[6];    // En Hafif .. Maksimum

    // Card 2: Targets
    const wchar_t* targetsTunneledFmt;// "TÜNELLENEN HEDEFLER (%d)"
    const wchar_t* targetsExcludedFmt;// "HARİÇ TUTULAN HEDEFLER (%d)"
    const wchar_t* targetsFullTitle;  // "TAM SİSTEM KAPSAMI"
    const wchar_t* badgeOnlyListed;   // "Sadece bu liste açılır"
    const wchar_t* badgeExcluded;     // "Bu listeye dokunulmaz"
    const wchar_t* badgeFullActive;   // "Tüm trafik aktif"

    const wchar_t* cueAddDomain;      // "Tünellenecek alan adı yazın (örn: discord.com)..."
    const wchar_t* cueExclDomain;     // "Hariç tutulacak alan adı yazın (örn: banka.com)..."
    const wchar_t* btnAddSite;        // "+ Site Ekle"
    const wchar_t* btnAddExcl;        // "+ Hariç Tut"

    const wchar_t* emptyTunneledTitle;
    const wchar_t* emptyTunneledSub;
    const wchar_t* emptyExcludedTitle;
    const wchar_t* emptyExcludedSub;
    const wchar_t* fullSystemTitle;
    const wchar_t* fullSystemSub;

    const wchar_t* tagSite;
    const wchar_t* tagExe;
    const wchar_t* tagFolder;

    const wchar_t* btnAddExe;         // "+ Uygulama (.exe)"
    const wchar_t* btnAddFolder;      // "+ Klasör Seç"
    const wchar_t* btnRemoveSel;      // "Seçiliyi Kaldır"

    // Card 3: Bottom Switches & Start/Stop
    const wchar_t* chkAutoConn;       // "Oto Bağlan"
    const wchar_t* chkWinStart;       // "Windows ile Aç"
    const wchar_t* chkMinStart;       // "Gizli Başlat"
    const wchar_t* btnStart;          // "▶  Başlat"
    const wchar_t* btnStop;           // "■  Durdur"

    // Tray & Dialogs
    const wchar_t* trayOpen;
    const wchar_t* trayStart;
    const wchar_t* trayStop;
    const wchar_t* trayExit;
    const wchar_t* dlgPickExeTunnel;
    const wchar_t* dlgPickExeExcl;
    const wchar_t* dlgPickFolderTunnel;
    const wchar_t* dlgPickFolderExcl;
};

static const UiStrings g_locales[LANG_ID_COUNT] = {
    // 0: TR — Türkçe
    {
        L"tr", L"Türkçe (TR)", L"TR",
        L"●  Aktif • Kademe %d", L"○  Hazır • %.1f MB",
        L"ÇALIŞMA KAPSAMI",
        L"Sadece Liste", L"Seçili hedefleri tünelle",
        L"Hariç Tutulanlar", L"Liste dışındakileri aç",
        L"Tüm Sistem", L"Her şeyi koşulsuz aç",
        L"DPI KADEMESİ", L"Kutucuğa bastığın an geçiş yapar",
        { L"En Hafif", L"Hafif", L"Orta", L"Güçlü", L"Çok Güçlü", L"Maksimum" },
        L"TÜNELLENEN HEDEFLER (%d)", L"HARİÇ TUTULAN HEDEFLER (%d)", L"TAM SİSTEM KAPSAMI",
        L"Sadece bu liste açılır", L"Bu listeye dokunulmaz", L"Tüm trafik aktif",
        L"Tünellenecek alan adı yazın (örn: discord.com)...",
        L"Hariç tutulacak alan adı yazın (örn: banka.com)...",
        L"+ Site Ekle", L"+ Hariç Tut",
        L"Henüz tünellenecek hedef yok",
        L"Üstteki kutudan alan adı ekleyebilir veya aşağıdaki butonlarla uygulama ve klasör seçebilirsiniz.",
        L"Henüz hariç tutulan hedef yok",
        L"Tünel dışı bırakmak istediğiniz siteyi üstten veya uygulamayı alttan ekleyin.",
        L"Tüm Sistem Trafiği Tünelleniyor",
        L"Bu modda liste filtrelemesi kullanılmaz. Bilgisayarınızdaki tüm uygulamalar ve web siteleri doğrudan seçili DPI kademesi üzerinden açılır.",
        L"site", L"exe", L"klasör",
        L"+ Uygulama (.exe)", L"+ Klasör Seç", L"Seçiliyi Kaldır",
        L"Oto Bağlan", L"Windows ile Aç", L"Gizli Başlat",
        L"▶  Başlat", L"■  Durdur",
        L"Pencereyi Aç", L"Başlat", L"Durdur", L"Çıkış",
        L"Tünellenecek Uygulamayı (.exe) Seçin", L"Hariç Tutulacak Uygulamayı (.exe) Seçin",
        L"Tünellenecek Uygulama Klasörünü Seçin:", L"Hariç Tutulacak Uygulama Klasörünü Seçin:"
    },
    // 1: EN — English
    {
        L"en", L"English (EN)", L"EN",
        L"●  Active • Stage %d", L"○  Ready • %.1f MB",
        L"TUNNEL SCOPE",
        L"Listed Only", L"Tunnel selected targets",
        L"All Except Listed", L"Bypass everything else",
        L"Full System", L"Tunnel all system traffic",
        L"DPI STAGE", L"Switches instantly on click",
        { L"Lightest", L"Light", L"Medium", L"Strong", L"Very Strong", L"Maximum" },
        L"TUNNELED TARGETS (%d)", L"EXCLUDED TARGETS (%d)", L"FULL SYSTEM SCOPE",
        L"Only listed items", L"Listed items bypassed", L"All traffic active",
        L"Enter domain to tunnel (e.g. discord.com)...",
        L"Enter domain to exclude (e.g. bank.com)...",
        L"+ Add Site", L"+ Exclude",
        L"No tunneled targets yet",
        L"Add a domain above or select an application (.exe) or folder using the buttons below.",
        L"No excluded targets yet",
        L"Add a domain above or select an application below to exclude it from the tunnel.",
        L"All System Traffic is Tunneled",
        L"List filtering is disabled in this mode. All applications and websites connect through the selected DPI stage.",
        L"site", L"exe", L"folder",
        L"+ App (.exe)", L"+ Add Folder", L"Remove Selected",
        L"Auto Connect", L"Start with Windows", L"Start Minimized",
        L"▶  Start", L"■  Stop",
        L"Open Window", L"Start", L"Stop", L"Exit",
        L"Select Application (.exe) to Tunnel", L"Select Application (.exe) to Exclude",
        L"Select Application Folder to Tunnel:", L"Select Application Folder to Exclude:"
    },
    // 2: DE — Deutsch
    {
        L"de", L"Deutsch (DE)", L"DE",
        L"●  Aktiv • Stufe %d", L"○  Bereit • %.1f MB",
        L"TUNNEL-BEREICH",
        L"Nur Liste", L"Ausgewählte Ziele tunneln",
        L"Ausnahmen", L"Alles außer Liste öffnen",
        L"Gesamtes System", L"Gesamten Verkehr tunneln",
        L"DPI-STUFE", L"Wechselt sofort beim Klicken",
        { L"Minimal", L"Leicht", L"Mittel", L"Stark", L"Sehr Stark", L"Maximum" },
        L"GETUNNELTE ZIELE (%d)", L"AUSGENOMMENE ZIELE (%d)", L"GESAMTES SYSTEM",
        L"Nur diese Liste", L"Liste unberührt", L"Alle Verbindungen aktiv",
        L"Domain zum Tunneln eingeben (z.B. discord.com)...",
        L"Domain zum Ausschließen eingeben...",
        L"+ Website", L"+ Ausnahme",
        L"Noch keine getunnelten Ziele",
        L"Oben eine Domain eingeben oder unten eine App (.exe) bzw. einen Ordner auswählen.",
        L"Noch keine Ausnahmen",
        L"Oben eine Website oder unten eine App hinzufügen, die nicht getunnelt werden soll.",
        L"Gesamter Systemverkehr wird getunnelt",
        L"In diesem Modus werden alle Anwendungen und Websites über die gewählte DPI-Stufe geleitet.",
        L"site", L"exe", L"ordner",
        L"+ App (.exe)", L"+ Ordner", L"Entfernen",
        L"Auto-Verbinden", L"Mit Windows starten", L"Minimiert",
        L"▶  Starten", L"■  Stoppen",
        L"Fenster öffnen", L"Starten", L"Stoppen", L"Beenden",
        L"App (.exe) zum Tunneln wählen", L"App (.exe) zum Ausschließen wählen",
        L"Ordner zum Tunneln wählen:", L"Ordner zum Ausschließen wählen:"
    },
    // 3: FR — Français
    {
        L"fr", L"Français (FR)", L"FR",
        L"●  Actif • Niveau %d", L"○  Prêt • %.1f MB",
        L"PORTÉE DU TUNNEL",
        L"Liste Seule", L"Tunneler la sélection",
        L"Exclusions", L"Tout sauf la liste",
        L"Système Entier", L"Tout le trafic système",
        L"NIVEAU DPI", L"Bascule instantanément au clic",
        { L"Très Léger", L"Léger", L"Moyen", L"Fort", L"Très Fort", L"Maximum" },
        L"CIBLES TUNNELÉES (%d)", L"CIBLES EXCLUES (%d)", L"SYSTÈME ENTIER",
        L"Liste uniquement", L"Liste ignorée", L"Tout le trafic actif",
        L"Domaine à tunneler (ex: discord.com)...",
        L"Domaine à exclure (ex: banque.com)...",
        L"+ Ajouter", L"+ Exclure",
        L"Aucune cible tunnelée",
        L"Ajoutez un domaine ci-dessus ou sélectionnez une application (.exe) ou un dossier ci-dessous.",
        L"Aucune cible exclue",
        L"Ajoutez un site ci-dessus ou une application ci-dessous à exclure du tunnel.",
        L"Tout le trafic système est tunnelé",
        L"Toutes les applications et tous les sites web passent par le niveau DPI sélectionné.",
        L"site", L"exe", L"dossier",
        L"+ App (.exe)", L"+ Dossier", L"Supprimer",
        L"Connexion auto", L"Ouvrir avec Windows", L"Démarrer réduit",
        L"▶  Démarrer", L"■  Arrêter",
        L"Ouvrir", L"Démarrer", L"Arrêter", L"Quitter",
        L"Choisir l'application (.exe)", L"Choisir l'application (.exe) à exclure",
        L"Choisir le dossier à tunneler :", L"Choisir le dossier à exclure :"
    },
    // 4: ES — Español
    {
        L"es", L"Español (ES)", L"ES",
        L"●  Activo • Nivel %d", L"○  Listo • %.1f MB",
        L"ALCANCE DEL TÚNEL",
        L"Solo Lista", L"Tunelizar seleccionados",
        L"Excluidos", L"Abrir todo excepto lista",
        L"Todo el Sistema", L"Todo el tráfico activo",
        L"NIVEL DPI", L"Cambia al instante al hacer clic",
        { L"Mínimo", L"Ligero", L"Medio", L"Fuerte", L"Muy Fuerte", L"Máximo" },
        L"OBJETIVOS EN TÚNEL (%d)", L"OBJETIVOS EXCLUIDOS (%d)", L"SISTEMA COMPLETO",
        L"Solo esta lista", L"Lista sin tocar", L"Todo el tráfico activo",
        L"Dominio para tunelizar (ej: discord.com)...",
        L"Dominio para excluir (ej: banco.com)...",
        L"+ Añadir", L"+ Excluir",
        L"Aún no hay objetivos en túnel",
        L"Añada un dominio arriba o seleccione una aplicación (.exe) o carpeta abajo.",
        L"Aún no hay objetivos excluidos",
        L"Añada un sitio arriba o aplicación abajo para excluir del túnel.",
        L"Todo el tráfico del sistema está en túnel",
        L"Todas las aplicaciones y sitios web se abren directamente mediante el nivel DPI seleccionado.",
        L"sitio", L"exe", L"carpeta",
        L"+ App (.exe)", L"+ Carpeta", L"Eliminar",
        L"Auto Conectar", L"Iniciar con Windows", L"Minimizado",
        L"▶  Iniciar", L"■  Detener",
        L"Abrir ventana", L"Iniciar", L"Detener", L"Salir",
        L"Seleccionar aplicación (.exe)", L"Seleccionar aplicación (.exe) a excluir",
        L"Seleccionar carpeta:", L"Seleccionar carpeta a excluir:"
    },
    // 5: PT — Português
    {
        L"pt", L"Português (PT)", L"PT",
        L"●  Ativo • Nível %d", L"○  Pronto • %.1f MB",
        L"ESCOPO DO TÚNEL",
        L"Apenas Lista", L"Tunelar selecionados",
        L"Excluídos", L"Abrir tudo exceto lista",
        L"Sistema Inteiro", L"Todo o tráfego ativo",
        L"NÍVEL DPI", L"Muda instantaneamente ao clicar",
        { L"Mínimo", L"Leve", L"Médio", L"Forte", L"Muito Forte", L"Máximo" },
        L"ALVOS TUNELADOS (%d)", L"ALVOS EXCLUÍDOS (%d)", L"SISTEMA INTEIRO",
        L"Apenas esta lista", L"Lista ignorada", L"Todo o tráfego ativo",
        L"Digite o domínio para tunelar (ex: discord.com)...",
        L"Digite o domínio para excluir...",
        L"+ Adicionar", L"+ Excluir",
        L"Nenhum alvo tunelado ainda",
        L"Adicione um domínio acima ou selecione um aplicativo (.exe) ou pasta abaixo.",
        L"Nenhum alvo excluído ainda",
        L"Adicione um site acima ou aplicativo abaixo para manter fora do túnel.",
        L"Todo o Tráfego do Sistema Tunelado",
        L"Todos os aplicativos e sites são abertos diretamente pelo nível DPI selecionado.",
        L"site", L"exe", L"pasta",
        L"+ App (.exe)", L"+ Pasta", L"Remover",
        L"Auto Conectar", L"Iniciar com Windows", L"Minimizado",
        L"▶  Iniciar", L"■  Parar",
        L"Abrir Janela", L"Iniciar", L"Parar", L"Sair",
        L"Selecionar aplicativo (.exe)", L"Selecionar aplicativo (.exe) para excluir",
        L"Selecionar pasta:", L"Selecionar pasta para excluir:"
    },
    // 6: IT — Italiano
    {
        L"it", L"Italiano (IT)", L"IT",
        L"●  Attivo • Livello %d", L"○  Pronto • %.1f MB",
        L"AMBITO TUNNEL",
        L"Solo Lista", L"Tunnel solo selezionati",
        L"Esclusi", L"Apri tutto tranne lista",
        L"Intero Sistema", L"Tutto il traffico attivo",
        L"LIVELLO DPI", L"Cambia istantaneamente al clic",
        { L"Minimo", L"Leggero", L"Medio", L"Forte", L"Molto Forte", L"Massimo" },
        L"OBIETTIVI IN TUNNEL (%d)", L"OBIETTIVI ESCLUSI (%d)", L"INTERO SISTEMA",
        L"Solo questa lista", L"Lista esclusa", L"Tutto il traffico attivo",
        L"Dominio da instradare (es. discord.com)...",
        L"Dominio da escludere...",
        L"+ Aggiungi", L"+ Escludi",
        L"Nessun obiettivo in tunnel",
        L"Aggiungi un dominio sopra o seleziona un'app (.exe) o cartella qui sotto.",
        L"Nessun obiettivo escluso",
        L"Aggiungi un sito sopra o un'app sotto per escluderli dal tunnel.",
        L"Tutto il Traffico di Sistema è in Tunnel",
        L"Tutte le applicazioni e i siti web passano attraverso il livello DPI selezionato.",
        L"sito", L"exe", L"cartella",
        L"+ App (.exe)", L"+ Cartella", L"Rimuovi",
        L"Auto Connetti", L"Avvia con Windows", L"Minimizzato",
        L"▶  Avvia", L"■  Ferma",
        L"Apri finestra", L"Avvia", L"Ferma", L"Esci",
        L"Seleziona applicazione (.exe)", L"Seleziona applicazione (.exe) da escludere",
        L"Seleziona cartella:", L"Seleziona cartella da escludere:"
    },
    // 7: RU — Русский
    {
        L"ru", L"Русский (RU)", L"RU",
        L"●  Активен • Уровень %d", L"○  Готов • %.1f МБ",
        L"РЕЖИМ ОХВАТА",
        L"Только список", L"Туннель выбранных целей",
        L"Исключения", L"Всё, кроме списка",
        L"Вся система", L"Весь трафик системы",
        L"УРОВЕНЬ DPI", L"Мгновенное переключение по клику",
        { L"Мин.", L"Лёгкий", L"Средний", L"Сильный", L"Оч. сильный", L"Максимум" },
        L"ЦЕЛИ В ТУННЕЛЕ (%d)", L"ИСКЛЮЧЁННЫЕ ЦЕЛИ (%d)", L"ВСЯ СИСТЕМА",
        L"Только этот список", L"Список не трогать", L"Весь трафик активен",
        L"Введите домен для обхода (напр. discord.com)...",
        L"Введите домен для исключения...",
        L"+ Добавить", L"+ Исключить",
        L"Нет целей для туннелирования",
        L"Добавьте домен выше или выберите приложение (.exe) или папку кнопками ниже.",
        L"Нет исключённых целей",
        L"Добавьте сайт выше или приложение ниже для исключения из туннеля.",
        L"Весь трафик системы туннелируется",
        L"Все приложения и сайты на компьютере открываются через выбранный уровень DPI.",
        L"сайт", L"exe", L"папка",
        L"+ Прил. (.exe)", L"+ Папка", L"Удалить",
        L"Автозапуск", L"Вместе с Windows", L"Свёрнуто",
        L"▶  Старт", L"■  Стоп",
        L"Открыть окно", L"Старт", L"Стоп", L"Выход",
        L"Выберите приложение (.exe)", L"Выберите приложение (.exe) для исключения",
        L"Выберите папку:", L"Выберите папку для исключения:"
    },
    // 8: JA — 日本語
    {
        L"ja", L"日本語 (JA)", L"JA",
        L"●  有効 • レベル %d", L"○  待機中 • %.1f MB",
        L"トンネル範囲",
        L"リストのみ", L"選択した対象のみ",
        L"除外リスト", L"リスト以外をすべて開放",
        L"システム全体", L"すべての通信を開放",
        L"DPI レベル", L"クリックで即座に切り替わります",
        { L"最軽量", L"軽量", L"標準", L"強力", L"超強力", L"最大" },
        L"トンネル対象 (%d)", L"除外対象 (%d)", L"システム全体",
        L"このリストのみ", L"リストを除外", L"全トラフィック有効",
        L"ドメインを入力 (例: discord.com)...",
        L"除外するドメインを入力...",
        L"+ サイト追加", L"+ 除外追加",
        L"トンネル対象がありません",
        L"上のボックスからドメインを追加するか、下のボタンでアプリ (.exe) やフォルダを選択してください。",
        L"除外対象がありません",
        L"トンネルから除外するサイトやアプリを追加してください。",
        L"システム全体の通信をトンネル中",
        L"すべてのアプリとウェブサイトが選択した DPI レベル経由で接続されます。",
        L"サイト", L"exe", L"フォルダ",
        L"+ アプリ (.exe)", L"+ フォルダ", L"選択を削除",
        L"自動接続", L"Windows起動時", L"最小化起動",
        L"▶  開始", L"■  停止",
        L"ウィンドウを開く", L"開始", L"停止", L"終了",
        L"アプリ (.exe) を選択", L"除外するアプリ (.exe) を選択",
        L"フォルダを選択:", L"除外するフォルダを選択:"
    },
    // 9: KO — 한국어
    {
        L"ko", L"한국어 (KO)", L"KO",
        L"●  활성 • 단계 %d", L"○  대기 • %.1f MB",
        L"터널링 범위",
        L"목록만 적용", L"선택한 대상만 우회",
        L"제외 목록", L"목록 외 모든 연결 우회",
        L"전체 시스템", L"모든 시스템 트래픽 우회",
        L"DPI 단계", L"클릭 즉시 실시간 전환됩니다",
        { L"최소", L"약함", L"보통", L"강함", L"매우 강함", L"최대" },
        L"터널링 대상 (%d)", L"제외된 대상 (%d)", L"전체 시스템 범위",
        L"이 목록만 우회", L"목록 제외", L"모든 트래픽 활성",
        L"우회할 도메인 입력 (예: discord.com)...",
        L"제외할 도메인 입력...",
        L"+ 사이트 추가", L"+ 제외 추가",
        L"등록된 터널링 대상이 없습니다",
        L"위 입력창에 도메인을 추가하거나 아래 버튼으로 앱(.exe) 또는 폴더를 선택하세요.",
        L"등록된 제외 대상이 없습니다",
        L"터널에서 제외할 사이트나 앱을 추가하세요.",
        L"전체 시스템 트래픽 터널링 중",
        L"모든 애플리케이션과 웹사이트가 선택한 DPI 단계를 통해 연결됩니다.",
        L"사이트", L"exe", L"폴더",
        L"+ 앱 (.exe)", L"+ 폴더 선택", L"선택 삭제",
        L"자동 연결", L"Windows 시작 시", L"최소화 시작",
        L"▶  시작", L"■  중지",
        L"창 열기", L"시작", L"중지", L"종료",
        L"터널링할 앱(.exe) 선택", L"제외할 앱(.exe) 선택",
        L"폴더 선택:", L"제외할 폴더 선택:"
    },
    // 10: ZH — 中文
    {
        L"zh", L"中文 (ZH)", L"ZH",
        L"●  运行中 • 等级 %d", L"○  就绪 • %.1f MB",
        L"代理范围",
        L"仅限列表", L"仅隧道选定目标",
        L"排除列表", L"代理列表外所有连接",
        L"全局系统", L"无条件代理全部流量",
        L"DPI 等级", L"点击即可实时切换等级",
        { L"极轻", L"轻度", L"中等", L"强力", L"超强", L"极限" },
        L"隧道目标 (%d)", L"排除目标 (%d)", L"全局系统模式",
        L"仅打开此列表", L"跳过此列表", L"全局流量已激活",
        L"输入要穿透的域名 (例如: discord.com)...",
        L"输入要排除的域名...",
        L"+ 添加站点", L"+ 添加排除",
        L"暂无隧道目标",
        L"在上方输入域名，或使用下方按钮选择应用程序 (.exe) 或文件夹。",
        L"暂无排除目标",
        L"添加要排除在隧道之外的网站或应用程序。",
        L"正在代理全部系统流量",
        L"此模式下不使用列表过滤，所有应用和网站均通过所选 DPI 等级直接连接。",
        L"站点", L"程序", L"文件夹",
        L"+ 应用 (.exe)", L"+ 选择文件夹", L"移除选中",
        L"自动连接", L"开机自启", L"静默启动",
        L"▶  启动", L"■  停止",
        L"打开窗口", L"启动", L"停止", L"退出",
        L"选择要代理的应用 (.exe)", L"选择要排除的应用 (.exe)",
        L"选择文件夹:", L"选择要排除的文件夹:"
    },
    // 11: AR — العربية
    {
        L"ar", L"العربية (AR)", L"AR",
        L"●  نشط • مستوى %d", L"○  جاهز • %.1f MB",
        L"نطاق العمل",
        L"القائمة فقط", L"تمرير الأهداف المحددة",
        L"المستثناة", L"فتح كل شيء عدا القائمة",
        L"كل النظام", L"تمرير جميع الاتصالات",
        L"مستوى DPI", L"التبديل فوري عند النقر",
        { L"الأخف", L"خفيف", L"متوسط", L"قوي", L"قوي جداً", L"الأقصى" },
        L"الأهداف الممررة (%d)", L"الأهداف المستثناة (%d)", L"نطاق النظام الكامل",
        L"هذه القائمة فقط", L"تجاهل القائمة", L"كل الاتصالات نشطة",
        L"أدخل النطاق (مثل discord.com)...",
        L"أدخل النطاق لاستثنائه...",
        L"+ إضافة موقع", L"+ استثناء",
        L"لا توجد أهداف بعد",
        L"أضف نطاقاً في الأعلى أو اختر تطبيقاً (.exe) أو مجلداً بالأسفل.",
        L"لا توجد استثناءات بعد",
        L"أضف موقعاً أو تطبيقاً لإبقائه خارج النفق.",
        L"يتم تمرير جميع اتصالات النظام",
        L"تعمل جميع التطبيقات والمواقع مباشرة عبر مستوى DPI المحدد.",
        L"موقع", L"تطبيق", L"مجلد",
        L"+ تطبيق (.exe)", L"+ مجلد", L"إزالة المحدد",
        L"اتصال تلقائي", L"البدء مع ويندوز", L"بدء مصغر",
        L"▶  بدء", L"■  إيقاف",
        L"فتح النافذة", L"بدء", L"إيقاف", L"خروج",
        L"اختر تطبيقاً (.exe)", L"اختر تطبيقاً لاستثنائه",
        L"اختر مجلداً:", L"اختر مجلداً لاستثنائه:"
    },
    // 12: NL — Nederlands
    {
        L"nl", L"Nederlands (NL)", L"NL",
        L"●  Actief • Niveau %d", L"○  Gereed • %.1f MB",
        L"TUNNELBEREIK",
        L"Alleen Lijst", L"Geselecteerde doelen",
        L"Uitgesloten", L"Alles behalve lijst",
        L"Hele Systeem", L"Al het verkeer actief",
        L"DPI-NIVEAU", L"Schakelt direct bij klikken",
        { L"Lichtst", L"Licht", L"Middel", L"Sterk", L"Zeer Sterk", L"Maximum" },
        L"GETUNNELDE DOELEN (%d)", L"UITGESLOTEN DOELEN (%d)", L"HELE SYSTEEM",
        L"Alleen deze lijst", L"Lijst genegeerd", L"Al het verkeer actief",
        L"Voer domein in (bijv. discord.com)...",
        L"Voer uit te sluiten domein in...",
        L"+ Toevoegen", L"+ Uitsluiten",
        L"Nog geen getunnelde doelen",
        L"Voeg hierboven een domein toe of kies hieronder een app (.exe) of map.",
        L"Nog geen uitgesloten doelen",
        L"Voeg een website of app toe om buiten de tunnel te houden.",
        L"Al het systeemverkeer wordt getunneld",
        L"Alle applicaties en websites lopen via het geselecteerde DPI-niveau.",
        L"site", L"exe", L"map",
        L"+ App (.exe)", L"+ Map", L"Verwijderen",
        L"Auto-verbinden", L"Start met Windows", L"Geminimaliseerd",
        L"▶  Starten", L"■  Stoppen",
        L"Venster openen", L"Starten", L"Stoppen", L"Afsluiten",
        L"Selecteer applicatie (.exe)", L"Selecteer uit te sluiten app (.exe)",
        L"Selecteer map:", L"Selecteer uit te sluiten map:"
    },
    // 13: PL — Polski
    {
        L"pl", L"Polski (PL)", L"PL",
        L"●  Aktywny • Poziom %d", L"○  Gotowy • %.1f MB",
        L"ZAKRES TUNELU",
        L"Tylko Lista", L"Tuneluj wybrane cele",
        L"Wykluczone", L"Wszystko poza listą",
        L"Cały System", L"Cały ruch systemowy",
        L"POZIOM DPI", L"Przełącza natychmiast po kliknięciu",
        { L"Najlżejszy", L"Lekki", L"Średni", L"Silny", L"B. Silny", L"Maksimum" },
        L"TUNELOWANE CELE (%d)", L"WYKLUCZONE CELE (%d)", L"CAŁY SYSTEM",
        L"Tylko ta lista", L"Lista pomijana", L"Cały ruch aktywny",
        L"Wpisz domenę do tunelowania (np. discord.com)...",
        L"Wpisz domenę do wykluczenia...",
        L"+ Dodaj", L"+ Wyklucz",
        L"Brak tunelowanych celów",
        L"Dodaj domenę powyżej lub wybierz aplikację (.exe) albo folder poniżej.",
        L"Brak wykluczonych celów",
        L"Dodaj stronę lub aplikację, aby pominąć ją w tunelu.",
        L"Cały Ruch Systemowy Tunelowany",
        L"Wszystkie aplikacje i strony otwierają się przez wybrany poziom DPI.",
        L"strona", L"exe", L"folder",
        L"+ Aplikacja (.exe)", L"+ Folder", L"Usuń wybrany",
        L"Auto-łączenie", L"Start z Windows", L"Zminimalizowany",
        L"▶  Start", L"■  Stop",
        L"Otwórz okno", L"Start", L"Stop", L"Wyjdź",
        L"Wybierz aplikację (.exe)", L"Wybierz aplikację (.exe) do wykluczenia",
        L"Wybierz folder:", L"Wybierz folder do wykluczenia:"
    },
    // 14: HI — हिन्दी
    {
        L"hi", L"हिन्दी (HI)", L"HI",
        L"●  सक्रिय • स्तर %d", L"○  तैयार • %.1f MB",
        L"टनल दायरा",
        L"केवल सूची", L"चयनित लक्ष्यों को खोलें",
        L"छोड़े गए", L"सूची के अलावा सब खोलें",
        L"संपूर्ण सिस्टम", L"सभी ट्रैफ़िक सक्रिय",
        L"DPI स्तर", L"क्लिक करते ही तुरंत बदलता है",
        { L"सबसे हल्का", L"हल्का", L"मध्यम", L"मज़बूत", L"बहुत मज़बूत", L"अधिकतम" },
        L"टनल लक्ष्य (%d)", L"छोड़े गए लक्ष्य (%d)", L"संपूर्ण सिस्टम",
        L"केवल यह सूची", L"सूची अनदेखी", L"सभी ट्रैफ़िक सक्रिय",
        L"डोमेन दर्ज करें (जैसे discord.com)...",
        L"छोड़ने के लिए डोमेन दर्ज करें...",
        L"+ साइट जोड़ें", L"+ बाहर रखें",
        L"अभी कोई टनल लक्ष्य नहीं है",
        L"ऊपर डोमेन जोड़ें या नीचे दिए गए बटन से ऐप (.exe) या फ़ोल्डर चुनें।",
        L"अभी कोई छोड़ा गया लक्ष्य नहीं है",
        L"टनल से बाहर रखने के लिए साइट या ऐप जोड़ें।",
        L"संपूर्ण सिस्टम ट्रैफ़िक टनल हो रहा है",
        L"सभी ऐप्स और वेबसाइटें चयनित DPI स्तर के माध्यम से खुलती हैं।",
        L"साइट", L"exe", L"फ़ोल्डर",
        L"+ ऐप (.exe)", L"+ फ़ोल्डर", L"चयनित हटाएं",
        L"ऑटो कनेक्ट", L"Windows के साथ खुलें", L"छोटा कर शुरू करें",
        L"▶  शुरू करें", L"■  रोकें",
        L"विंडो खोलें", L"शुरू करें", L"रोकें", L"बाहर निकलें",
        L"ऐप (.exe) चुनें", L"बाहर रखने के लिए ऐप (.exe) चुनें",
        L"फ़ोल्डर चुनें:", L"बाहर रखने के लिए फ़ोल्डर चुनें:"
    },
    // 15: ID — Bahasa Indonesia
    {
        L"id", L"Indonesia (ID)", L"ID",
        L"●  Aktif • Tahap %d", L"○  Siap • %.1f MB",
        L"CAKUPAN TUNNEL",
        L"Hanya Daftar", L"Tunnel target terpilih",
        L"Dikecualikan", L"Buka semua selain daftar",
        L"Seluruh Sistem", L"Semua trafik aktif",
        L"TAHAP DPI", L"Beralih seketika saat diklik",
        { L"Paling Ringan", L"Ringan", L"Sedang", L"Kuat", L"Sangat Kuat", L"Maksimum" },
        L"TARGET DITUNNEL (%d)", L"TARGET DIKECUALIKAN (%d)", L"SELURUH SISTEM",
        L"Hanya daftar ini", L"Daftar dilewati", L"Semua trafik aktif",
        L"Masukkan domain (mis. discord.com)...",
        L"Masukkan domain yang dikecualikan...",
        L"+ Tambah Situs", L"+ Kecualikan",
        L"Belum ada target tunnel",
        L"Tambahkan domain di atas atau pilih aplikasi (.exe) atau folder di bawah.",
        L"Belum ada target dikecualikan",
        L"Tambahkan situs di atas atau aplikasi di bawah untuk dikecualikan.",
        L"Semua Trafik Sistem Ditunnel",
        L"Semua aplikasi dan situs web dibuka langsung melalui tahap DPI yang dipilih.",
        L"situs", L"exe", L"folder",
        L"+ Aplikasi (.exe)", L"+ Pilih Folder", L"Hapus Terpilih",
        L"Auto Konek", L"Buka saat Startup", L"Mulai Minimized",
        L"▶  Mulai", L"■  Berhenti",
        L"Buka Jendela", L"Mulai", L"Berhenti", L"Keluar",
        L"Pilih Aplikasi (.exe)", L"Pilih Aplikasi (.exe) untuk Dikecualikan",
        L"Pilih Folder:", L"Pilih Folder untuk Dikecualikan:"
    },
    // 16: TH — ไทย
    {
        L"th", L"ไทย (TH)", L"TH",
        L"●  ทำงาน • ระดับ %d", L"○  พร้อม • %.1f MB",
        L"ขอบเขตการทำงาน",
        L"เฉพาะรายการ", L"ทะลุบล็อกเฉพาะที่เลือก",
        L"รายการยกเว้น", L"เปิดทั้งหมดเว้นรายการนี้",
        L"ทั้งระบบ", L"เปิดทุกการเชื่อมต่อ",
        L"ระดับ DPI", L"สลับทันทีเมื่อคลิก",
        { L"เบาที่สุด", L"เบา", L"ปานกลาง", L"แรง", L"แรงมาก", L"สูงสุด" },
        L"เป้าหมายที่ทะลุบล็อก (%d)", L"เป้าหมายที่ยกเว้น (%d)", L"ครอบคลุมทั้งระบบ",
        L"เฉพาะรายการนี้", L"ไม่แตะต้องรายการนี้", L"ทราฟฟิกทั้งหมดทำงาน",
        L"ป้อนโดเมน (เช่น discord.com)...",
        L"ป้อนโดเมนที่ต้องการยกเว้น...",
        L"+ เพิ่มเว็บ", L"+ ยกเว้น",
        L"ยังไม่มีเป้าหมาย",
        L"เพิ่มโดเมนด้านบน หรือเลือกแอป (.exe) และโฟลเดอร์ด้วยปุ่มด้านล่าง",
        L"ยังไม่มีรายการยกเว้น",
        L"เพิ่มเว็บไซต์หรือแอปที่ไม่ต้องการให้ผ่านอุโมงค์",
        L"กำลังทะลุบล็อกทราฟฟิกทั้งระบบ",
        L"แอปพลิเคชันและเว็บไซต์ทั้งหมดจะเชื่อมต่อผ่านระดับ DPI ที่เลือกโดยตรง",
        L"เว็บ", L"exe", L"โฟลเดอร์",
        L"+ แอป (.exe)", L"+ เลือกโฟลเดอร์", L"ลบที่เลือก",
        L"เชื่อมต่ออัตโนมัติ", L"เปิดพร้อม Windows", L"ย่อหน้าต่างเริ่ม",
        L"▶  เริ่ม", L"■  หยุด",
        L"เปิดหน้าต่าง", L"เริ่ม", L"หยุด", L"ออก",
        L"เลือกแอปพลิเคชัน (.exe)", L"เลือกแอปพลิเคชัน (.exe) ที่ยกเว้น",
        L"เลือกโฟลเดอร์:", L"เลือกโฟลเดอร์ที่ยกเว้น:"
    },
    // 17: VI — Tiếng Việt
    {
        L"vi", L"Tiếng Việt (VI)", L"VI",
        L"●  Đang chạy • Mức %d", L"○  Sẵn sàng • %.1f MB",
        L"PHẠM VI ĐƯỜNG HẦM",
        L"Chỉ Danh Sách", L"Mở các mục đã chọn",
        L"Loại Trừ", L"Mở tất cả trừ danh sách",
        L"Toàn Hệ Thống", L"Mở toàn bộ lưu lượng",
        L"MỨC ĐỘ DPI", L"Chuyển đổi ngay khi nhấp chuột",
        { L"Rất Nhẹ", L"Nhẹ", L"Vừa", L"Mạnh", L"Rất Mạnh", L"Tối Đa" },
        L"MỤC TIÊU ĐƯỜNG HẦM (%d)", L"MỤC TIÊU LOẠI TRỪ (%d)", L"TOÀN HỆ THỐNG",
        L"Chỉ danh sách này", L"Bỏ qua danh sách", L"Mọi lưu lượng hoạt động",
        L"Nhập tên miền (VD: discord.com)...",
        L"Nhập tên miền muốn loại trừ...",
        L"+ Thêm Web", L"+ Loại Trừ",
        L"Chưa có mục tiêu nào",
        L"Thêm tên miền ở trên hoặc chọn ứng dụng (.exe) / thư mục ở dưới.",
        L"Chưa có mục loại trừ",
        L"Thêm trang web hoặc ứng dụng muốn bỏ qua đường hầm.",
        L"Đang Mở Toàn Bộ Lưu Lượng Hệ Thống",
        L"Tất cả ứng dụng và trang web được kết nối trực tiếp qua mức DPI đã chọn.",
        L"web", L"exe", L"thư mục",
        L"+ Ứng dụng (.exe)", L"+ Thư mục", L"Xóa mục chọn",
        L"Tự kết nối", L"Mở cùng Windows", L"Chạy ẩn",
        L"▶  Bắt đầu", L"■  Dừng",
        L"Mở cửa sổ", L"Bắt đầu", L"Dừng", L"Thoát",
        L"Chọn ứng dụng (.exe)", L"Chọn ứng dụng (.exe) loại trừ",
        L"Chọn thư mục:", L"Chọn thư mục loại trừ:"
    },
    // 18: UK — Українська
    {
        L"uk", L"Українська (UK)", L"UK",
        L"●  Активний • Рівень %d", L"○  Готовий • %.1f МБ",
        L"РЕЖИМ ОХОПЛЕННЯ",
        L"Лише список", L"Тунелювати вибрані цілі",
        L"Винятки", L"Усе, крім списку",
        L"Уся система", L"Увесь трафік системи",
        L"РІВЕНЬ DPI", L"Миттєве перемикання при натисканні",
        { L"Мін.", L"Легкий", L"Середній", L"Сильний", L"Дуже сильний", L"Максимум" },
        L"ЦІЛІ В ТУНЕЛІ (%d)", L"ВИКЛЮЧЕНІ ЦІЛІ (%d)", L"УСЯ СИСТЕМА",
        L"Лише цей список", L"Список не чіпати", L"Увесь трафік активний",
        L"Введіть домен для обходу (напр. discord.com)...",
        L"Введіть домен для виключення...",
        L"+ Додати", L"+ Виключити",
        L"Немає цілей для тунелювання",
        L"Додайте домен вище або виберіть програму (.exe) чи папку нижче.",
        L"Немає виключених цілей",
        L"Додайте сайт вище або програму нижче для виключення з тунелю.",
        L"Увесь трафік системи тунелюється",
        L"Усі програми та сайти відкриваються безпосередньо через вибраний рівень DPI.",
        L"сайт", L"exe", L"папка",
        L"+ Програма (.exe)", L"+ Папка", L"Видалити",
        L"Автопідключення", L"Разом із Windows", L"Згорнуто",
        L"▶  Старт", L"■  Стоп",
        L"Відкрити вікно", L"Старт", L"Стоп", L"Вихід",
        L"Виберіть програму (.exe)", L"Виберіть програму (.exe) для виключення",
        L"Виберіть папку:", L"Виберіть папку для виключення:"
    }
};

static inline AppLangId DetectSystemDefaultLang() {
    LANGID lid = GetUserDefaultUILanguage();
    WORD prim = PRIMARYLANGID(lid);
    switch (prim) {
        case LANG_TURKISH:    return LANG_ID_TR;
        case LANG_GERMAN:     return LANG_ID_DE;
        case LANG_FRENCH:     return LANG_ID_FR;
        case LANG_SPANISH:    return LANG_ID_ES;
        case LANG_PORTUGUESE: return LANG_ID_PT;
        case LANG_ITALIAN:    return LANG_ID_IT;
        case LANG_RUSSIAN:    return LANG_ID_RU;
        case LANG_JAPANESE:   return LANG_ID_JA;
        case LANG_KOREAN:     return LANG_ID_KO;
        case LANG_CHINESE:    return LANG_ID_ZH;
        case LANG_ARABIC:     return LANG_ID_AR;
        case LANG_DUTCH:      return LANG_ID_NL;
        case LANG_POLISH:     return LANG_ID_PL;
        case LANG_HINDI:      return LANG_ID_HI;
        case LANG_INDONESIAN: return LANG_ID_ID;
        case LANG_THAI:       return LANG_ID_TH;
        case LANG_VIETNAMESE: return LANG_ID_VI;
        case LANG_UKRAINIAN:  return LANG_ID_UK;
        default:              return LANG_ID_EN;
    }
}

static inline AppLangId ParseLangCode(const wchar_t* code) {
    if (!code || !code[0]) return DetectSystemDefaultLang();
    for (int i = 0; i < LANG_ID_COUNT; ++i) {
        if (_wcsicmp(code, g_locales[i].code) == 0) {
            return (AppLangId)i;
        }
    }
    return DetectSystemDefaultLang();
}

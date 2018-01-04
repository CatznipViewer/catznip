; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Italian.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_ITALIAN} "Linguaggio del programma di installazione"
LangString SelectInstallerLanguage  ${LANG_ITALIAN} "Scegliere per favore il linguaggio del programma di installazione"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_ITALIAN} " Update"
LangString LicenseSubTitleSetup ${LANG_ITALIAN} " Setup"

; installation directory text
LangString DirectoryChooseTitle ${LANG_ITALIAN} "Directory di installazione" 
LangString DirectoryChooseUpdate ${LANG_ITALIAN} "Scegli la directory di Catznip per l’update alla versione ${VERSION_LONG}.(XXX):"
LangString DirectoryChooseSetup ${LANG_ITALIAN} "Scegli la directory dove installare Catznip:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_ITALIAN} "Non riesco a trovare il programma '$INSTPROG'. Silent Update fallito."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_ITALIAN} "Avvia ora Catznip?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_ITALIAN} "Controllo delle precedenti versioni…"

; check windows version
LangString CheckWindowsVersionDP ${LANG_ITALIAN} "Controllo della versione di Windows…"
LangString CheckWindowsVersionMB ${LANG_ITALIAN} 'Catznip supporta solo Windows Vista.$\n$\nTentare l’installazione su Windows $R0 può provocare blocchi di sistema e perdita di dati.$\n$\n'
LangString CheckWindowsServPackMB ${LANG_ITALIAN} "Si consiglia di avviare Catznip utilizzando il service pack più recente disponibile per il vostro sistema operativo. $\n Così facendo, il programma sarà più stabile e performante."
LangString UseLatestServPackDP ${LANG_ITALIAN} "Utilizza Windows Update per installare il Service Pak più recente."

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_ITALIAN} "Controllo del permesso di installazione…"
LangString CheckAdministratorInstMB ${LANG_ITALIAN} 'Stai utilizzando un account “limitato”.$\nSolo un “amministratore” può installare Catznip.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_ITALIAN} "Controllo del permesso di installazione…"
LangString CheckAdministratorUnInstMB ${LANG_ITALIAN} 'Stai utilizzando un account “limitato”.$\nSolo un “amministratore” può installare Catznip.'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_ITALIAN} "Catznip ${VERSION_LONG} è stato sia già installato.$\n$\nVuoi ripetere l’installazione?"

; checkcpuflags
LangString MissingSSE2 ${LANG_ITALIAN} "Questo computer potrebbe non avere un CPU con supporto SSE2, necessario per avviare Catznip ${VERSION_LONG}. Vuoi continuare?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_ITALIAN} "In attesa che Catznip chiuda…"
LangString CloseSecondLifeInstMB ${LANG_ITALIAN} "Non è possibile installare Catznip se è già in funzione..$\n$\nTermina le operazioni in corso e scegli OK per chiudere Catznip e continuare.$\nScegli CANCELLA per annullare l’installazione."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_ITALIAN} "In attesa della chiusura di Catznip…"
LangString CloseSecondLifeUnInstMB ${LANG_ITALIAN} "Non è possibile installare Catznip se è già in funzione.$\n$\nTermina le operazioni in corso e scegli OK per chiudere Catznip e continuare.$\nScegli CANCELLA per annullare."

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_ITALIAN} "Verifica connessione di rete in corso..."

; ask to remove user's data files
LangString RemoveDataFilesMB ${LANG_ITALIAN} "Vuoi inoltre RIMUOVERE tutti gli altri file connessi a Catznip? $\n$\n Ti consigliamo di conservare le impostazioni e i file della cache se hai un'altra versione di Catznip già installata o se stai disinstallando Catznip per aggiornarlo a una versione più  recente."

; delete program files
LangString DeleteProgramFilesMB ${LANG_ITALIAN} "Sono ancora presenti dei file nella directory programmi di Catznip.$\n$\nPotrebbe trattarsi di file creati o trasferiti in:$\n$INSTDIR$\n$\nVuoi cancellarli?"

; uninstall text
LangString UninstallTextMsg ${LANG_ITALIAN} "Così facendo Catznip verrà disinstallato ${VERSION_LONG} dal tuo sistema."

; ask to remove registry keys that still might be needed by other viewers that are installed
LangString DeleteRegistryKeysMB ${LANG_ITALIAN} "Vuoi rimuovere le chiavi di registro dell'applicazione?$\n$\n Ti consigliamo di conservare le chiavi di registro se hai un'altra versione di Catznip installata."
Problemi individuati a prima vista:
- Codice che non compila su nuovi sistemi -> Sistemare codice vecchio e chiamate alle librerie
- Usa ancora autotools, che ormai è obsoleto -> Conversione a CMake (o Meson)
- Documentazione vecchia e incompleta -> Aggiornare documentazione
- Presenza di strumenti che sembrano non utilizzati -> Revisionare quali tool sono ancora usati e rimuovere quelli non utilizzati (tipo xsis)
- Mancanza di tutorial di base ed esempi semplici -> Creazione di esempi
- Interfaccia a riga di comando piuttosto brutta e scomoda -> Cambiare l'interfaccia e utilizzare librerie apposite come GNU readline
## VERSIONE 1.1.0
**Gestione modulare delle dipendenze**
  - Deploy su github delle librerie condivise
  - version.h con definizioni di versione
  - version_check.h con logica di warning
  - Macro EXPECTED_LIB_VERSION_MAJOR/MINOR nei progetti

**Sottoscrizione dinamica MQTT**
  - Funzione sottoscriviTopics() con array di topic
  - Terminatore nullptr per flessibilità
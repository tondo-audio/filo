# PRD — Tondo Host
**Versione:** 0.1  
**Autore:** Tondo Audio  
**Stato:** Draft

---

## 1. Problema

I chitarristi che usano i plugin Tondo Audio non hanno un modo semplice per suonarli in tempo reale senza aprire una DAW. Le DAW sono overkill per il caso d'uso base: collegare la chitarra, mettere qualche plugin in catena, suonare sopra un video di YouTube.

Le alternative open source esistenti (Carla, Pedalboard2, Element) sono troppo complesse per un utente non tecnico.

---

## 2. Soluzione

**Tondo Host** è un'applicazione standalone macOS, leggera e open source, pensata esclusivamente per chitarristi che vogliono suonare in tempo reale attraverso una catena di plugin AU/VST3.

L'interfaccia ha un unico schermo. Nessun routing visuale, nessun patchbay, nessun concetto DAW.

---

## 3. Utente target

Chitarrista non tecnico che:
- Registra demo a casa
- Si esercita suonando sopra backing track o video YouTube
- Usa i plugin Tondo Audio (Timbro, Spazio, ecc.)
- Non vuole imparare a usare una DAW

---

## 4. Casi d'uso

### UC-1 — Sessione base
1. L'utente apre Tondo Host
2. Seleziona l'interfaccia audio (input chitarra, output cuffie/casse)
3. Aggiunge uno o più plugin alla catena
4. Suona in tempo reale
5. Chiude l'app

### UC-2 — Cambio plugin al volo
1. L'utente ha già la catena attiva
2. Rimuove un plugin o ne aggiunge uno nuovo
3. Il suono cambia senza interruzioni

### UC-3 — Suonare sopra YouTube
1. L'utente apre YouTube nel browser
2. Apre Tondo Host in parallelo
3. Il suono della chitarra processato e l'audio del video escono insieme dalle stesse cuffie

---

## 5. Requisiti funzionali

### RF-1 — Selezione audio
- L'utente sceglie il dispositivo di input (canale chitarra)
- L'utente sceglie il dispositivo di output (cuffie o casse)
- Entrambi possono stare sulla stessa interfaccia audio
- La selezione è persistente al riavvio

### RF-2 — Catena plugin
- L'utente può aggiungere plugin AU e VST3 presenti nel sistema
- I plugin vengono processati in ordine dall'alto verso il basso
- L'utente può riordinare i plugin con drag & drop
- L'utente può rimuovere un plugin dalla catena
- Ogni plugin può essere bypassato con un click

### RF-3 — Interfaccia del plugin
- Ogni plugin mostra la propria UI nativa quando si fa doppio click
- La finestra del plugin è indipendente dalla finestra principale

### RF-4 — Latenza
- Latenza target ≤ 10ms con buffer size 128 samples su CoreAudio
- L'utente può regolare il buffer size nelle preferenze

### RF-5 — Scan plugin
- Al primo avvio l'app scansiona le cartelle standard AU e VST3
- L'utente può avviare una nuova scansione manualmente

---

## 6. Requisiti non funzionali

- **Piattaforma:** macOS 12+ (Apple Silicon + Intel), Windows 10+, Linux (Ubuntu 22+)
- **Formato:** App standalone (.app), distribuita via download diretto o GitHub Releases
- **Open source:** Licenza MIT
- **Framework:** JUCE 8
- **Dimensione:** < 20MB
- **Avvio:** < 2 secondi a freddo

---

## 7. Fuori scope (v1.0)

- ❌ Salvataggio sessioni/preset della catena
- ❌ Registrazione audio
- ❌ MIDI
- ❌ Windows e Linux
- ❌ Plugin in formato VST2 o LADSPA
- ❌ Routing multi-canale
- ❌ Metering avanzato

---

## 8. UI — Schermo unico

```
┌─────────────────────────────────────┐
│  🎸 Tondo Host                      │
├─────────────────────────────────────┤
│  INPUT   [Focusrite 2i2 - Ch.1  ▾]  │
│  OUTPUT  [Focusrite 2i2 - Mix   ▾]  │
│  BUFFER  [128 samples           ▾]  │
├─────────────────────────────────────┤
│  CATENA                  [+ Aggiungi]│
│  ┌─────────────────────────────────┐│
│  │ ◉  Timbro          [UI] [bypass]││
│  │ ◉  Spazio          [UI] [bypass]││
│  └─────────────────────────────────┘│
│                                     │
│  ████████████████░░░░  INPUT  -12dB │
│  ████████████████░░░░  OUTPUT -12dB │
└─────────────────────────────────────┘
```

- Nessun menu complesso
- Nessun patchbay o routing visuale
- Meter input/output per capire se il segnale passa
- Drag & drop per riordinare i plugin nella catena

---

## 9. Stack tecnico

| Componente | Scelta |
|---|---|
| Framework | JUCE 8 |
| Audio engine | `AudioProcessorGraph` |
| Device management | `AudioDeviceManager` (CoreAudio) |
| Plugin loading | `AudioPluginFormatManager` (AU + VST3) |
| UI | JUCE Components, tema Tondo Audio |
| Build | CMake + Xcode (Mac), CMake + MSVC (Win), CMake + GCC (Linux) |
| Distribuzione | GitHub Releases (.dmg Mac, .exe Win, .AppImage Linux) |

---

## 10. Milestone

| Fase | Contenuto | Stima |
|---|---|---|
| **M1** | Audio passthrough (input → output senza plugin) | 2 giorni |
| **M2** | Caricamento e catena di un singolo plugin | 3 giorni |
| **M3** | Catena multipla con drag & drop e bypass | 3 giorni |
| **M4** | Scan plugin, UI nativa, meter | 3 giorni |
| **M5** | Polish, icona, .dmg, GitHub Release | 2 giorni |

**Totale stimato:** ~2-3 settimane part-time

---

## 11. Decisioni prese

| Domanda | Risposta |
|---|---|
| Plugin scan | Solo path standard di sistema, nessun path custom |
| Icona | Distinta da Tondo Audio, da progettare |
| Catena al primo avvio | Vuota — l'utente aggiunge i plugin manualmente |
| Piattaforma | macOS, Windows, Linux (tutte e tre) |

### Path standard per plugin scan

**AU (macOS only)**
- `/Library/Audio/Plug-Ins/Components`
- `~/Library/Audio/Plug-Ins/Components`

**VST3 (tutte le piattaforme)**
- macOS: `/Library/Audio/Plug-Ins/VST3` e `~/Library/Audio/Plug-Ins/VST3`
- Windows: `C:\Program Files\Common Files\VST3`
- Linux: `~/.vst3` e `/usr/lib/vst3` e `/usr/local/lib/vst3`

# Negate

Negate è uno strumento da linea di comando per la generazione di negativi digitali da file **CR3** (Canon RAW), con salvataggio in **PNG** ad alta qualità. Permette la regolazione avanzata di luminosità, contrasto, gamma, livelli, colore e altri parametri, offrendo un flusso di lavoro flessibile per la post-produzione di negativi fotografici.

---

## Funzionalità principali

- **Supporto CR3** tramite ImageMagick/Magick++ (richiede supporto RAW).
- **Negazione** opzionale dell’immagine.
- **Regolazione automatica** di luminosità e contrasto in base alle statistiche dell’immagine.
- **Controllo avanzato** di gamma, livelli, bilanciamento del bianco, saturazione, tinta, sharpening e altro.
- **Output PNG** senza perdita di qualità.
- **Conversione in scala di grigi** opzionale.
- **Interfaccia a riga di comando** con molte opzioni configurabili.

---

## Requisiti

- **CMake** >= 3.10
- **GCC/G++** >= 14 consigliato
- **ImageMagick** con supporto Magick++ e RAW (CR3)
- (Facoltativo) File CR3 di esempio

---

## Installazione

```bash
git clone <URL_DEL_REPOSITORY>
cd negate
cmake .
make
```

---

## Utilizzo

```bash
./negate [opzioni] [input.cr3] [output.png]
```

Se non specificati, i file di input/output predefiniti sono `immagine.cr3` e `output_negativo.png`.

### Opzioni disponibili

- `-i=FILE`, `--input=FILE`      File di input (default: `immagine.cr3`)
- `-o=FILE`, `--output=FILE`     File di output (default: `output_negativo.png`)
- `-n`, `--negate`           Applica la negazione dell’immagine
- `-c`, `--color`           Mantieni i colori (output non in scala di grigi)
- `-t=VAL`, `--target=VAL`     Luminosità target (default: 0.45)
- `-g=VAL`, `--gamma=VAL`     Valore gamma (default: 1.225)
- `-l=VAL`, `--level-max=VAL`   Valore massimo per level (default: 0.98)
- `-k=VAL`, `--level-gamma=VAL`  Gamma per level (default: 1.2)
- `--wb-red=VAL`          Bilanciamento bianco rosso (default: 1.05)
- `--wb-green=VAL`        Bilanciamento bianco verde (default: 0.98)
- `--wb-blue=VAL`         Bilanciamento bianco blu (default: 1.02)
- `--sigmoidal-strength=VAL`   Forza sigmoidalContrast (default: 2.5)
- `--sigmoidal-midpoint=VAL`   Punto medio sigmoidalContrast (default: 0.4)
- `--saturation=VAL`       Saturazione per modulate (default: 115.0)
- `--hue=VAL`           Tinta per modulate (default: 97.0)
- `--evaluate-factor=VAL`    Fattore per evaluate (default: 0.2)
- `--sharpen-sigma=VAL`     Sigma per adaptiveSharpen (default: 0.5)
- `-h`, `--help`          Mostra l’aiuto

### Esempio

```bash
./negate -i=immagine.cr3 -o=negativo.png -n -g=1.3 -c --wb-red=1.1 --saturation=120
```

---

## Output

Il file PNG risultante viene salvato con il nome specificato (default: `output_negativo.png`).  
Il programma non modifica mai il file originale.

---

## Note

- Il supporto ai file CR3 dipende dalla versione di ImageMagick installata.
- Alcuni parametri avanzati sono pensati per utenti esperti di fotografia digitale e post-produzione.

---

## Crediti

- **ImageMagick/Magick++** per l’elaborazione immagini
- Basato su idee e struttura CR3 da [Canon CR3](https://github.com/lclevy/canon_cr3)

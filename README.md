# Negate

Negate è uno strumento da linea di comando per elaborare file **CR3** (Canon RAW), generando negativi in scala di grigi o a colori e salvando il risultato come **PNG** ad alta qualità. L'applicazione permette di regolare luminosità, contrasto, gamma e altri parametri tramite opzioni dedicate.

---

## Funzionalità

- **Lettura file CR3** tramite ImageMagick (Magick++).
- **Negazione** opzionale dell'immagine.
- **Regolazione automatica** di luminosità e contrasto in base alle statistiche dell'immagine.
- **Controllo parametri**: gamma, livello massimo, luminosità target, mantenimento colore.
- **Output PNG** senza perdita di qualità.
- **Interfaccia a riga di comando** con opzioni flessibili.

---

## Requisiti

- **CMake** >= 3.10
- **GCC/G++** (consigliato >= 14)
- **ImageMagick** con supporto Magick++
- (Opzionale) File CR3 di esempio per test

---

## Installazione

1. Clona il repository:
   ```bash
   git clone <URL_DEL_REPOSITORY>
   cd negate
   ```
2. Configura il progetto:
   ```bash
   cmake .
   ```
3. Compila:
   ```bash
   make
   ```

---

## Utilizzo

Posiziona un file `.cr3` nella directory del progetto e lancia:

```bash
./negate [opzioni] [input.cr3] [output.png]
```

### Opzioni principali

- `-i=FILE`, `--input=FILE`   File di input (default: `immagine.cr3`)
- `-o=FILE`, `--output=FILE`  File di output (default: `output_negativo.png`)
- `-n`, `--negate`        Applica la negazione dell'immagine
- `-t=VAL`, `--target=VAL`   Luminosità target (default: 0.45)
- `-g=VAL`, `--gamma=VAL`   Valore gamma (default: 1.225)
- `-l=VAL`, `--level-max=VAL` Valore massimo per level (default: 0.98)
- `-k=VAL`, `--level-gamma=VAL` Gamma per level (default: 1.2)
- `-c`, `--color`        Mantieni i colori (output non in scala di grigi)
- `-h`, `--help`        Mostra l'aiuto

Esempio:

```bash
./negate -i=immagine.cr3 -o=negativo.png -n -g=1.3 -c
```

---

## Output

Il file PNG risultante sarà salvato con il nome specificato (default: `output_negativo.png`).

---

## Note

- Il supporto ai file CR3 dipende dalla versione di ImageMagick installata.
- Il programma non modifica i file originali.

---

## Crediti

- **ImageMagick/Magick++**: Elaborazione immagini e supporto RAW
- Basato su idee e struttura CR3 da [Canon CR3](https://github.com/lclevy/canon_cr3)

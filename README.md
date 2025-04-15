# Negate CR3 Image Project

Un'applicazione per leggere file in bianco e nero **CR3** (Canon RAW), generare un negativo e salvarlo come **PNG** alla massima risoluzione.

---

## Caratteristiche

- **Lettura CR3**: Usa **LibRaw** per estrarre dati RAW.
- **Generazione Negativo**: Trasformazione di immagini bianco e nero.
- **Salvataggio PNG**: Utilizzo di **libpng** per qualità senza perdita.

---

## Prerequisiti

- **CMake** >= 3.10
- **GCC/G++**
- **LibRaw**
- **libpng**

---

## Installazione

1. Clona il repository:
   ```bash
   git clone <URL_DEL_REPOSITORY>
   cd negate
   ```
2. Configura con CMake:
   ```bash
   cmake .
   ```
3. Compila:
   ```bash
   make
   ```

## Utilizzo

Posiziona un file CR3 nella directory del progetto e usa il comando:

```bash
./negate immagine.cr3
```

Il file PNG risultante sarà `output_negativo.png`.

---

## Prossimi sviluppi

- Conversione a **DNG**
- Scrittura diretta di file CR3 (magari!)

---

## Crediti

- **LibRaw**: Gestione file RAW
- **libpng**: Salvataggio PNG
- Struttura CR3 basata su [Canon CR3](https://github.com/lclevy/canon_cr3)

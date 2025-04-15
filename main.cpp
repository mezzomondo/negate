#include <libraw/libraw.h>
#include <iostream>

int main() {
    LibRaw processor;

    // Apri un file CR3
    if (processor.open_file("immagine.cr3") != LIBRAW_SUCCESS) {
        std::cerr << "Errore nell'apertura del file CR3" << std::endl;
        return 1;
    }

    // Estrai i dati RAW
    if (processor.unpack() != LIBRAW_SUCCESS) {
        std::cerr << "Errore nell'estrazione dei dati RAW" << std::endl;
        return 1;
    }

    std::cout << "Larghezza: " << processor.imgdata.sizes.width
              << ", Altezza: " << processor.imgdata.sizes.height << std::endl;

    // Elaborazione dati qui...

    return 0;
}

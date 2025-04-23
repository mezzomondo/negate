// Inclusione delle librerie standard e di ImageMagick, ordinate alfabeticamente
#include <filesystem> // Per la gestione dei percorsi dei file
#include <iostream>   // Per l'input/output su console
#include <Magick++.h> // Per l'elaborazione delle immagini con ImageMagick
#include <stdexcept>  // Per la gestione delle eccezioni
#include <string>     // Per la manipolazione delle stringhe
#include <utility>    // Per std::pair
#include <vector>     // Per la gestione di contenitori dinamici

// Namespace e definizioni globali
namespace fs = std::filesystem;             // Alias per il namespace filesystem
using namespace Magick;                     // Namespace di ImageMagick per semplificare l'uso delle sue classi
inline constexpr bool debug_enabled = true; // Abilita i messaggi di debug su stderr

// Struttura per contenere le opzioni del programma, definite tramite riga di comando
struct ProgramOptions
{
    std::string inputFile = "immagine.cr3";         // Percorso del file di input (default: immagine.cr3)
    std::string outputFile = "output_negativo.png"; // Percorso del file di output (default: output_negativo.png)
    double targetLuma = 0.45;                       // Luminosità target per la regolazione della brightness
    double contrastThreshold = 0.20;                // Soglia di contrasto per decidere la regolazione della luminosità
    double gamma = 1.225;                           // Valore di correzione gamma per bilanciare la luminosità percepita
    double levelMax = 0.98;                         // Valore massimo per la regolazione dei livelli
    double levelGamma = 1.2;                        // Gamma per la regolazione dei livelli
    bool applyNegate = false;                       // Flag per applicare la negazione dell'immagine
    bool keepColor = false;                         // Flag per mantenere i colori o convertire in scala di grigi
    double wb_red = 1.05;                           // Fattore di bilanciamento del bianco per il canale rosso
    double wb_green = 0.98;                         // Fattore di bilanciamento del bianco per il canale verde
    double wb_blue = 1.02;                          // Fattore di bilanciamento del bianco per il canale blu
    double sigmoidal_strength = 2.5;                // Forza della regolazione tonale sigmoidalContrast
    double sigmoidal_midpoint = 0.4;                // Punto medio della regolazione tonale sigmoidalContrast
    double saturation = 115.0;                      // Percentuale di saturazione per la modulazione del colore
    double hue = 97.0;                              // Percentuale di tinta per la modulazione del colore
    double evaluate_factor = 0.2;                   // Fattore per l'ottimizzazione delle ombre/luci
    double sharpen_sigma = 0.5;                     // Sigma per lo sharpening adattivo
};

// Funzione di utilità per stampare messaggi di debug su stderr
template <typename... Args>
void debug(Args &&...args)
{
    if constexpr (debug_enabled)
    {
        // Concatena tutti gli argomenti e aggiunge una nuova linea
        (std::cerr << ... << args) << '\n';
    }
}

// Funzione per mostrare l'uso corretto del programma e le opzioni disponibili
void showUsage(const char *programName)
{
    // Stampa il messaggio di utilizzo con tutte le opzioni e i loro valori di default
    std::cerr << "Uso: " << programName << " [opzioni]\n"
              << "Opzioni:\n"
              << "  -i, --input=FILE             File di input (default: immagine.cr3)\n"
              << "  -o, --output=FILE            File di output (default: output_negativo.png)\n"
              << "  -n, --negate                 Applica negazione dell'immagine\n"
              << "  -c, --color                  Mantieni i colori (default: scala di grigi)\n"
              << "  -t, --target=VAL             Luminosità target (default: 0.45)\n"
              << "  -g, --gamma=VAL              Valore gamma (default: 1.225)\n"
              << "  -l, --level-max=VAL          Valore massimo per level (default: 0.98)\n"
              << "  -k, --level-gamma=VAL        Gamma per level (default: 1.2)\n"
              << "  --wb-red=VAL                 Bilanciamento bianco rosso (default: 1.05)\n"
              << "  --wb-green=VAL               Bilanciamento bianco verde (default: 0.98)\n"
              << "  --wb-blue=VAL                Bilanciamento bianco blu (default: 1.02)\n"
              << "  --sigmoidal-strength=VAL     Forza sigmoidalContrast (default: 2.5)\n"
              << "  --sigmoidal-midpoint=VAL     Punto medio sigmoidalContrast (default: 0.4)\n"
              << "  --saturation=VAL             Saturazione per modulate (default: 115.0)\n"
              << "  --hue=VAL                    Tinta per modulate (default: 97.0)\n"
              << "  --evaluate-factor=VAL        Fattore per evaluate (default: 0.2)\n"
              << "  --sharpen-sigma=VAL          Sigma per adaptiveSharpen (default: 0.5)\n"
              << "  -h, --help                   Mostra questo messaggio\n";
}

// Funzione per calcolare la media e la deviazione standard di un'immagine in scala di grigi
std::pair<double, double> computeImageStatistics(const Image &inputImage, bool applyNegate)
{
    // Crea una copia dell'immagine in scala di grigi per evitare di modificare l'originale
    Image grayImage = inputImage;
    grayImage.colorSpace(GRAYColorspace);
    debug("Creata copia in scala di grigi per le statistiche");

    // Applica la negazione alla copia in scala di grigi, se richiesto
    if (applyNegate)
    {
        grayImage.negate();
        debug("Applicata negazione alla copia in scala di grigi");
    }

    // Calcola le statistiche dell'immagine in scala di grigi
    ImageStatistics stats = grayImage.statistics();
    ChannelStatistics grayStats = stats.channel(RedPixelChannel);

    // Normalizza la media e la deviazione standard rispetto a QuantumRange
    double grayMean = grayStats.mean() / QuantumRange;
    double grayStd = grayStats.standardDeviation() / QuantumRange;

    // Restituisce una coppia contenente media e deviazione standard
    return {grayMean, grayStd};
}

// Funzione per il parsing degli argomenti da riga di comando
ProgramOptions parseArguments(int argc, char *argv[])
{
    // Inizializza la struttura delle opzioni con i valori di default
    ProgramOptions options;

    // Itera sugli argomenti da riga di comando
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        // Mostra l'help e termina se richiesto
        if (arg == "-h" || arg == "--help")
        {
            showUsage(argv[0]);
            exit(0);
        }
        // Abilita la negazione dell'immagine
        else if (arg == "-n" || arg == "--negate")
        {
            options.applyNegate = true;
        }
        // Mantieni i colori invece di convertire in scala di grigi
        else if (arg == "-c" || arg == "--color")
        {
            options.keepColor = true;
        }
        // Specifica il file di input
        else if (arg.find("--input=") == 0)
        {
            options.inputFile = arg.substr(8);
        }
        else if (arg.find("-i=") == 0)
        {
            options.inputFile = arg.substr(3);
        }
        // Specifica il file di output
        else if (arg.find("--output=") == 0)
        {
            options.outputFile = arg.substr(9);
        }
        else if (arg.find("-o=") == 0)
        {
            options.outputFile = arg.substr(3);
        }
        // Imposta la luminosità target
        else if (arg.find("--target=") == 0)
        {
            options.targetLuma = std::stod(arg.substr(9));
        }
        else if (arg.find("-t=") == 0)
        {
            options.targetLuma = std::stod(arg.substr(3));
        }
        // Imposta il valore di gamma
        else if (arg.find("--gamma=") == 0)
        {
            options.gamma = std::stod(arg.substr(8));
        }
        else if (arg.find("-g=") == 0)
        {
            options.gamma = std::stod(arg.substr(3));
        }
        // Imposta il valore massimo per la regolazione dei livelli
        else if (arg.find("--level-max=") == 0)
        {
            options.levelMax = std::stod(arg.substr(12));
        }
        else if (arg.find("-l=") == 0)
        {
            options.levelMax = std::stod(arg.substr(3));
        }
        // Imposta il gamma per la regolazione dei livelli
        else if (arg.find("--level-gamma=") == 0)
        {
            options.levelGamma = std::stod(arg.substr(14));
        }
        else if (arg.find("-k=") == 0)
        {
            options.levelGamma = std::stod(arg.substr(3));
        }
        // Imposta il bilanciamento del bianco per il canale rosso
        else if (arg.find("--wb-red=") == 0)
        {
            options.wb_red = std::stod(arg.substr(9));
        }
        // Imposta il bilanciamento del bianco per il canale verde
        else if (arg.find("--wb-green=") == 0)
        {
            options.wb_green = std::stod(arg.substr(11));
        }
        // Imposta il bilanciamento del bianco per il canale blu
        else if (arg.find("--wb-blue=") == 0)
        {
            options.wb_blue = std::stod(arg.substr(10));
        }
        // Imposta la forza della regolazione tonale sigmoidalContrast
        else if (arg.find("--sigmoidal-strength=") == 0)
        {
            options.sigmoidal_strength = std::stod(arg.substr(20));
        }
        // Imposta il punto medio della regolazione tonale sigmoidalContrast
        else if (arg.find("--sigmoidal-midpoint=") == 0)
        {
            options.sigmoidal_midpoint = std::stod(arg.substr(20));
        }
        // Imposta la saturazione per la modulazione del colore
        else if (arg.find("--saturation=") == 0)
        {
            options.saturation = std::stod(arg.substr(13));
        }
        // Imposta la tinta per la modulazione del colore
        else if (arg.find("--hue=") == 0)
        {
            options.hue = std::stod(arg.substr(6));
        }
        // Imposta il fattore per l'ottimizzazione delle ombre/luci
        else if (arg.find("--evaluate-factor=") == 0)
        {
            options.evaluate_factor = std::stod(arg.substr(18));
        }
        // Imposta il sigma per lo sharpening adattivo
        else if (arg.find("--sharpen-sigma=") == 0)
        {
            options.sharpen_sigma = std::stod(arg.substr(16));
        }
        // Gestione degli argomenti posizionali per input/output
        else if (options.inputFile == "immagine.cr3")
        {
            options.inputFile = arg;
        }
        else if (options.outputFile == "output_negativo.png")
        {
            options.outputFile = arg;
        }
        // Errore per opzioni non riconosciute
        else
        {
            std::cerr << "Opzione non riconosciuta: " << arg << "\n";
            showUsage(argv[0]);
            exit(1);
        }
    }

    // Registra tutte le opzioni parsate per il debug
    debug("File di input: ", options.inputFile);
    debug("File di output: ", options.outputFile);
    debug("Target luma: ", options.targetLuma);
    debug("Gamma: ", options.gamma);
    debug("Level max: ", options.levelMax);
    debug("Level gamma: ", options.levelGamma);
    debug("Applica negazione: ", options.applyNegate ? "sì" : "no");
    debug("Mantieni i colori: ", options.keepColor ? "sì" : "no");
    debug("Bilanciamento bianco rosso: ", options.wb_red);
    debug("Bilanciamento bianco verde: ", options.wb_green);
    debug("Bilanciamento bianco blu: ", options.wb_blue);
    debug("Forza sigmoidalContrast: ", options.sigmoidal_strength);
    debug("Punto medio sigmoidalContrast: ", options.sigmoidal_midpoint);
    debug("Saturazione: ", options.saturation);
    debug("Tinta: ", options.hue);
    debug("Fattore evaluate: ", options.evaluate_factor);
    debug("Sigma sharpen: ", options.sharpen_sigma);

    return options;
}

// Funzione per calcolare il fattore di modulazione della luminosità
double computeBrightnessModulate(
    double grayMean,
    double grayStdDev,
    double targetLuma,
    double contrastThreshold,
    double maxBoostPercent)
{
    // Evita divisioni per zero o valori non validi
    if (grayMean < 1e-5)
    {
        debug("Gray mean troppo basso, restituito maxBoostPercent: ", maxBoostPercent);
        return maxBoostPercent;
    }

    // Se il contrasto è alto, riduce leggermente la luminosità target
    if (grayStdDev > contrastThreshold)
    {
        targetLuma -= 0.03;
        debug("Contrasto alto, targetLuma ridotto a: ", targetLuma);
    }

    // Calcola il fattore di modulazione con un esponente per attenuare l'effetto
    double factor = std::pow((targetLuma / grayMean), 0.75);
    double brightness = std::min(factor * 100.0, maxBoostPercent);

    // Registra il valore calcolato per il debug
    debug("Brightness modulate calcolata: ", brightness);
    return brightness;
}

// Funzione principale per l'elaborazione dell'immagine
int main(int argc, char *argv[])
{
    try
    {
        // Inizializza la libreria ImageMagick
        InitializeMagick(nullptr);
        debug("ImageMagick inizializzato");

        // Parsa gli argomenti da riga di comando
        ProgramOptions options = parseArguments(argc, argv);
        debug("Argomenti da riga di comando parsati");

        // Carica l'immagine originale dal file specificato
        Image originalImage(options.inputFile);
        debug("Immagine originale caricata: ", options.inputFile);

        // FASE 1: Calcolo delle statistiche iniziali
        // Calcola la media e la deviazione standard dell'immagine originale per analizzare la luminosità e il contrasto iniziali
        auto [initialGrayMean, initialGrayStd] = computeImageStatistics(originalImage, options.applyNegate);
        debug("Statistiche iniziali calcolate: mean=", initialGrayMean, ", std=", initialGrayStd);

        // Crea una copia dell'immagine originale per le trasformazioni
        Image finalImage = originalImage;
        debug("Creata copia dell'immagine per le trasformazioni");

        // FASE 2: Applicazione delle trasformazioni di base
        // Applica una serie di trasformazioni per migliorare il colore, il contrasto e la nitidezza

        // 2.1. Bilanciamento del bianco: corregge le dominanti di colore regolando i canali RGB
        double wb_matrix[] = {options.wb_red, 0, 0, 0, options.wb_green, 0, 0, 0, options.wb_blue};
        finalImage.colorMatrix(3, wb_matrix);
        debug("Applicato bilanciamento del bianco: R=", options.wb_red, ", G=", options.wb_green, ", B=", options.wb_blue);

        // 2.2 Regolazione tonale: applica un contrasto sigmoide per migliorare i toni senza clipping
        finalImage.sigmoidalContrast(true, options.sigmoidal_strength, options.sigmoidal_midpoint);
        debug("Applicata regolazione tonale sigmoidalContrast: strength=", options.sigmoidal_strength, ", midpoint=", options.sigmoidal_midpoint);

        // 2.3. Modulazione del colore: regola la saturazione e la tinta per migliorare la vivacità
        finalImage.modulate(100.0, options.saturation, options.hue);
        debug("Applicata modulazione del colore: saturation=", options.saturation, ", hue=", options.hue);

        // 2.4. Ottimizzazione delle ombre/luci: regola l'intensità dei pixel e normalizza la gamma dinamica
        finalImage.evaluate(AllChannels, MultiplyEvaluateOperator, options.evaluate_factor);
        finalImage.normalize();
        debug("Applicata ottimizzazione delle ombre/luci: evaluate_factor=", options.evaluate_factor);

        // 2.5. Sharpening adattivo: migliora la nitidezza dei dettagli con un filtro adattivo
        finalImage.adaptiveSharpen(0, options.sharpen_sigma);
        debug("Applicato sharpening adattivo: sigma=", options.sharpen_sigma);

        // FASE 3: Calcolo delle statistiche post-trasformazione
        // Calcola le statistiche sull'immagine trasformata per valutare l'effetto delle modifiche
        auto [transformedGrayMean, transformedGrayStd] = computeImageStatistics(finalImage, options.applyNegate);
        debug("Statistiche post-trasformazione calcolate: mean=", transformedGrayMean, ", std=", transformedGrayStd);

        // FASE 4: Applicazione condizionale delle trasformazioni di regolazione fine
        // Decide se applicare ulteriori regolazioni in base alla luminosità e al contrasto dell'immagine trasformata

        // Condizione: applica le regolazioni se la luminosità media è fuori dall'intervallo [0.3, 0.7] o il contrasto è basso (< 0.15)
        bool applyAdjustments = (transformedGrayMean < 0.3 || transformedGrayMean > 0.7 || transformedGrayStd < 0.15);
        debug("Trasformazioni di regolazione fine: ", applyAdjustments ? "applicate" : "non applicate");

        if (applyAdjustments)
        {
            // 4.1. Negazione: inverte i colori dell'immagine, se richiesto
            if (options.applyNegate)
            {
                finalImage.negate();
                debug("Applicata negazione all'immagine finale");
            }

            // 4.2. Calcolo della modulazione della luminosità: determina il fattore di brightness per raggiungere la luminosità target
            double brightness = computeBrightnessModulate(transformedGrayMean, transformedGrayStd,
                                                          options.targetLuma, options.contrastThreshold, 150.0);

            // 4.3. Regolazione dei livelli: ottimizza la gamma dinamica dell'immagine
            finalImage.level(0.0, options.levelMax * QuantumRange, options.levelGamma);
            debug("Applicata regolazione dei livelli: max=", options.levelMax, ", gamma=", options.levelGamma);

            // 4.4. Correzione gamma: bilancia la luminosità percepita
            finalImage.gamma(options.gamma);
            debug("Applicata correzione gamma: ", options.gamma);

            // 4.5. Modulazione della luminosità: applica il fattore di brightness calcolato
            finalImage.modulate(brightness, 100.0, 100.0);
            debug("Applicata modulazione della luminosità: ", brightness);

            // 4.6. Miglioramento del contrasto: applica un miglioramento finale del contrasto
            finalImage.contrast(true);
            debug("Applicato miglioramento del contrasto");
        }
        else
        {
            // Se non si applicano le regolazioni, applica comunque la negazione se richiesta
            if (options.applyNegate)
            {
                finalImage.negate();
                debug("Applicata negazione all'immagine finale (senza trasformazioni di regolazione)");
            }
        }

        // FASE 5: Gestione del colore e salvataggio
        // Finalizza l'immagine e la salva nel formato specificato

        // Converti in scala di grigi se non si vuole mantenere il colore
        if (!options.keepColor)
        {
            finalImage.colorSpace(GRAYColorspace);
            debug("Immagine convertita in scala di grigi");
        }

        // Salva l'immagine elaborata nel file di output
        finalImage.write(options.outputFile);
        debug("Immagine elaborata salvata in: ", options.outputFile);
    }
    catch (const std::exception &e)
    {
        // Gestisce eventuali errori durante l'elaborazione
        std::cerr << "Errore: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
// Renders the plugin window to a PNG without opening it on screen.
// Usage: UISnapshot <out.png> [sample-file] [skin name]

#include "PluginEditor.h"
#include "PluginProcessor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2)
        return 2;

    std::unique_ptr<juce::AudioProcessor> proc (createPluginFilter());
    auto& tp = static_cast<ts::TurntableProcessor&> (*proc);
    proc->prepareToPlay (48000.0, 128);

    if (argc > 2)
    {
        tp.loadFile (juce::File (juce::String (argv[2])));
        for (int i = 0; i < 300 && tp.getUiSample() == nullptr; ++i)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (20);

        // Play a little so the platter and waveform show movement.
        tp.togglePlay();
        tp.storePad (0);
        juce::AudioBuffer<float> buf (2, 128);
        juce::MidiBuffer midi;
        for (int i = 0; i < 1500; ++i)
            proc->processBlock (buf, midi);
        tp.toggleLoop();
        juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc->createEditorIfNeeded());

    if (juce::String (argv[1]) == "--keytest")
    {
        // Replays a key sequence through the real editor and prints the deck state.
        juce::AudioBuffer<float> buf (2, 128);
        juce::MidiBuffer midi;
        auto runSeconds = [&] (double s) {
            for (int i = 0; i < (int) (s * 48000 / 128); ++i)
                proc->processBlock (buf, midi);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
        };
        auto press = [&] (const char* name, juce::KeyPress k) {
            const bool used = editor->keyPressed (k);
            editor->keyStateChanged (false);
            runSeconds (0.3);
            std::printf ("%-8s used=%d loopOn=%d in=%.0f out=%.0f pad0=%d playhead=%.0f playing=%d\n", name, (int) used,
                         (int) tp.isLoopOn(), tp.getLoopIn(), tp.getLoopOut(), (int) tp.padHasRegion (0),
                         tp.status.playhead.load(), (int) tp.status.playing.load());
        };
        const double beat = tp.barFrames() / 4.0;
        auto press2 = [&] (const char* name, juce::KeyPress k) {
            const bool used = editor->keyPressed (k);
            editor->keyStateChanged (false);
            runSeconds (0.05);
            std::printf ("%-26s used=%d loopOn=%d in=%.3f beats out=%.3f beats | pads:", name, (int) used, (int) tp.isLoopOn(),
                         tp.getLoopIn() / beat, tp.getLoopOut() / beat);
            for (int i = 0; i < 4; ++i)
                std::printf (" %d", (int) tp.padHasRegion (i));
            std::printf (" | playhead %.3f beats\n", tp.status.playhead.load() / beat);
        };
        const int shift = juce::ModifierKeys::shiftModifier, alt = juce::ModifierKeys::altModifier;
        runSeconds (1.13);
        press2 ("L (loop off)", juce::KeyPress ('L', 0, 'l'));
        runSeconds (1.37);
        press2 ("3 (empty pad: new loop)", juce::KeyPress ('3', 0, '3'));
        runSeconds (1.21);
        press2 ("Shift+2 (types \") store", juce::KeyPress ('"', shift, '"'));
        press2 ("L (off)", juce::KeyPress ('L', 0, 'l'));
        runSeconds (2.2);
        press2 ("I (loop in, snapped)", juce::KeyPress ('I', 0, 'i'));
        runSeconds (1.9);
        press2 ("O (loop out, snapped)", juce::KeyPress ('O', 0, 'o'));
        press2 ("L (on)", juce::KeyPress ('L', 0, 'l'));
        runSeconds (0.7);
        press2 ("Alt+5 (types [) loop in", juce::KeyPress ('5', alt, '['));
        press2 ("2 (recall pad 2)", juce::KeyPress ('2', 0, '2'));
        press2 ("H (help)", juce::KeyPress ('H', 0, 'h'));
        proc->editorBeingDeleted (editor.get());
        return 0;
    }

    if (argc > 3)
        if (auto* te = dynamic_cast<ts::TurntableEditor*> (editor.get()))
            te->applySkin (juce::String (argv[3]), false);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
    proc->editorBeingDeleted (editor.get());

    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (argv[1])));
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    const bool ok = stream.openedOk() && png.writeImageToStream (img, stream);
    editor.reset();
    return ok ? 0 : 1;
}

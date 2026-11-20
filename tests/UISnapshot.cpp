// Renders the plugin window to a PNG without opening it on screen.
// Usage: UISnapshot <out.png> [sample-file]

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
    juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
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

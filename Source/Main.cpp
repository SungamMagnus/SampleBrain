#include <juce_gui_basics/juce_gui_basics.h>
#include "MainComponent.h"

// Command line, for testing and for the README screenshot:
//   --target <file>      load a target sound
//   --sounds <dir|file>  add sounds to the brain
//   --generate           generate the brain and the target blocks after loading
//   --shot <png>         render the window to a PNG after a moment, then quit
class SamplebrainApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise (const juce::String& commandLine) override
    {
        window = std::make_unique<MainWindow> (getApplicationName());
        auto& main = *static_cast<MainComponent*> (window->getContentComponent());

        auto args = juce::StringArray::fromTokens (commandLine, true);
        args.trim();
        args.removeEmptyStrings();
        auto value = [&] (const char* flag) -> juce::String
        {
            const int i = args.indexOf (flag);
            return i >= 0 && i + 1 < args.size() ? args[i + 1].unquoted() : juce::String();
        };

        if (auto t = value ("--target"); t.isNotEmpty())
            main.loadTarget (juce::File::getCurrentWorkingDirectory().getChildFile (t));
        if (auto s = value ("--sounds"); s.isNotEmpty())
        {
            auto f = juce::File::getCurrentWorkingDirectory().getChildFile (s);
            if (f.isDirectory()) main.addFolder (f);
            else main.addSounds ({ f });
        }
        if (args.contains ("--generate"))
            main.generateAll();

        if (auto shot = value ("--shot"); shot.isNotEmpty())
        {
            const auto out = juce::File::getCurrentWorkingDirectory().getChildFile (shot);
            juce::Timer::callAfterDelay (args.contains ("--generate") ? 6000 : 1500, [this, out]
            {
                auto* c = window->getContentComponent();
                auto image = c->createComponentSnapshot (c->getLocalBounds(), true, 2.0f);
                out.deleteFile();
                if (auto stream = out.createOutputStream())
                    juce::PNGImageFormat().writeImageToStream (image, *stream);
                systemRequestedQuit();
            });
        }
    }

    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : juce::DocumentWindow (name, Sungam::paper, juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);
            setResizable (true, true);
            setResizeLimits (1040, 700, 4000, 3000);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override { JUCEApplication::getInstance()->systemRequestedQuit(); }
    };

private:
    std::unique_ptr<MainWindow> window;
};

START_JUCE_APPLICATION (SamplebrainApplication)

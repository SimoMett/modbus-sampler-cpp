#include <SDL2/SDL_opengl.h>
#include <iostream>
#include <cstdlib>

#include "GuiWorker.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl2.h"
#include "implot/implot.h"
#include "simomett/common.h"

GuiWorker::GuiWorker(std::shared_ptr<spdlog::logger> logger, std::string window_name, json gui_config, json tags) : should_close(false), is_running(false), logger(logger), refresh_rate_ms(gui_config["refresh_rate"].get<unsigned short>()), deque_max_len(gui_config["deque_max_len"].get<unsigned short>()), light_theme(gui_config["light_theme"].get<bool>())
{
    std::unordered_map<uint32_t, std::string> *maps[3];
    maps[MbValueType::WORD_TYPE] = &this->words_names;
    maps[MbValueType::DWORD_TYPE] = &this->dwords_names;
    maps[MbValueType::REAL_TYPE] = &this->floats_names;

    std::string json_str[3];
    json_str[MbValueType::WORD_TYPE] = "words";
    json_str[MbValueType::DWORD_TYPE] = "dwords";
    json_str[MbValueType::REAL_TYPE] = "floats";

    for (auto v : {MbValueType::WORD_TYPE, MbValueType::DWORD_TYPE, MbValueType::REAL_TYPE})
    {
        if (!tags[json_str[v]].is_null())
        {
            for (json s : tags[json_str[v]])
            {
                std::string formatted_tag_name = ConsumerWorker::format_name(s["tag"].get<std::string>());

                maps[v]->insert({s["address"].get<uint32_t>(), formatted_tag_name});
                this->samples_queues.insert({formatted_tag_name, SamplesRingQueue(deque_max_len, v)});
            }
        }
    }
}

GuiWorker::~GuiWorker()
{
}

void GuiWorker::start()
{
    run_thread = std::make_unique<std::thread>(&GuiWorker::run, this);
}

void GuiWorker::stop()
{
    this->should_close = true;
}

void GuiWorker::run()
{
    this->logger->info("Initializing Gui worker..");
    this->is_running = true;
    this->should_close = false;

    try
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
            throw std::runtime_error(std::string("Failed to SDL_Init. Error: ") + SDL_GetError());

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        float display_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        SDL_Window *window = SDL_CreateWindow(window_name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(1280 * display_scale), (int)(800 * display_scale), window_flags);
        if (window == nullptr)
            throw std::runtime_error(std::string("Error: SDL_CreateWindow(): ") + SDL_GetError());

        SDL_GLContext gl_context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, gl_context);
        SDL_GL_SetSwapInterval(1); // Enable vsync

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

        ImPlot::CreateContext();

        // Setup Dear ImGui style
        if (light_theme)
            ImGui::StyleColorsLight();
        else
            ImGui::StyleColorsDark();

        // Setup scaling
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(display_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
        style.FontScaleDpi = display_scale; // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
        ImGui_ImplOpenGL2_Init();

        // Our state
        const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // Main loop
        std::chrono::system_clock::time_point dump_timer = std::chrono::system_clock::now();
        while (!should_close)
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    should_close = true;
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    should_close = true;
            }
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
            {
                SDL_Delay(100);
                continue;
            }

            if (std::chrono::system_clock::now() - dump_timer > std::chrono::milliseconds(refresh_rate_ms))
            {
                this->dump_samples();
                dump_timer = std::chrono::system_clock::now();
            }

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            ImGui::SetNextWindowPos({0, 0});
            int tmpWidth, tmpHeight;
            SDL_GetWindowSize(window, &tmpWidth, &tmpHeight);
            ImGui::SetNextWindowSize({(float)tmpWidth, (float)tmpHeight});
            if (ImGui::Begin("Mainwindow", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
            {
                for (auto &kv : samples_queues)
                {
                    const char *tag_name = kv.first.c_str();
                    const SamplesRingQueue &queue = kv.second;
                    if (ImPlot::BeginPlot(tag_name, ImVec2(-1, 500), ImPlotFlags_NoTitle | ImPlotFlags_NoInputs))
                    {
                        const std::vector<float> & data_x = queue.x_data();
                        const std::vector<float> & data_y = queue.y_data();
                        const std::vector<const char *> labels = queue.data_labels();

                        /*if (data_x.size() >= 2)
                            ImPlot::SetupAxisTicks(ImAxis_X1, 0.0, (double)(data_x.size() - 1), data_x.size(), labels.data());*/
                        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
                        ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_AutoFit);

                        ImPlot::SetupAxis(ImAxis_Y1, "Value", ImPlotAxisFlags_AutoFit);

                        ImPlot::PlotLine(tag_name, data_x.data(), data_y.data(), data_x.size());
                        ImPlot::EndPlot();
                    }
                }
            }
            ImGui::End();

            // Rendering
            std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
            ImGui::Render();
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            // glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
            ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);

            std::chrono::nanoseconds delta = (std::chrono::system_clock::now() - start);
            std::chrono::milliseconds maxDelta = std::chrono::milliseconds(30);
            if (delta < maxDelta)
                std::this_thread::sleep_for(maxDelta - delta);
        }

        // Cleanup
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        ImPlot::DestroyContext();

        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();

        this->logger->info("Gui worker stopped");
    }
    catch (const std::exception &e)
    {
        this->logger->error(e.what());
        this->logger->error("Gui worker stopped due to error");
    }
    this->is_running = false;
}

void GuiWorker::join()
{
    this->run_thread->join();
}

bool GuiWorker::running()
{
    return this->is_running;
}

void GuiWorker::push_words(std::vector<AddressValue<uint16_t>> samples, std::chrono::system_clock::time_point time)
{
    for (auto &sample : samples)
    {
        std::string tag_name = words_names[sample.address];
        MbValue v;
        v.word = sample.val;
        // samples_queues[tag_name].append(Sample{v, time}); //do not use [] operator
        samples_queues.at(tag_name).append(Sample{v, time});
    }
}
void GuiWorker::push_floats(std::vector<AddressValue<float>> samples, std::chrono::system_clock::time_point time)
{
    for (auto &sample : samples)
    {
        std::string tag_name = floats_names[sample.address];
        MbValue v;
        v.real = sample.val;
        samples_queues.at(tag_name).append(Sample{v, time});
    }
}
void GuiWorker::push_dwords(std::vector<AddressValue<uint32_t>> samples, std::chrono::system_clock::time_point time)
{
    for (auto &sample : samples)
    {
        std::string tag_name = dwords_names[sample.address];
        MbValue v;
        v.dword = sample.val;
        samples_queues.at(tag_name).append(Sample{v, time});
    }
}

void GuiWorker::dump_samples()
{
    for (auto &kv : samples_queues)
    {
        SamplesRingQueue &queue = kv.second;
        queue.refresh();
    }
}
#include "ConsumerWorker.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "SamplesRingQueue.h"
#include <SFML/Graphics.hpp>
#include <pugixml.hpp>
#include "lunasvg.h"

using nlohmann::json;

class SFMLWorker: public ConsumerWorker
{
public:
    static const std::string WORKER_VERSION;
    
    SFMLWorker(std::shared_ptr<spdlog::logger> logger, std::string window_name, json gui_config, json tags);
    ~SFMLWorker();
    void start();
    void join();
    void stop();
    bool running();
    void push_words(std::vector<AddressValue<uint16_t>>, std::chrono::system_clock::time_point);
    void push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point);
    void push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point);
    void push_coils(std::vector<AddressValue<bool>>, std::chrono::system_clock::time_point);
    void push_bits(std::vector<BitAddressValue>, std::chrono::system_clock::time_point);

private:
    std::unique_ptr<std::thread> run_thread;
    bool should_close;
    bool is_running;
    const std::shared_ptr<spdlog::logger> logger;
    std::unordered_map<addr_t, std::string> words_names;
    std::unordered_map<addr_t, std::string> floats_names;
    std::unordered_map<addr_t, std::string> dwords_names;
    std::unordered_map<addr_t, std::string> coils_names;
    json gui_tags;
    const unsigned short fps_limit;
    std::unique_ptr<lunasvg::Document> document;
    sf::RenderWindow window;
    const std::string window_name;

    void run();
    void dump_samples();
    void self_close();

    static constexpr sf::Color background_color = sf::Color(46, 106, 201);
    static void patchDocument(pugi::xml_document & doc);
    static void setItemColor(std::unique_ptr<lunasvg::Document> & doc, const std::string & item_name, const std::string & color);
    static void setFieldText(std::unique_ptr<lunasvg::Document> & doc, const std::string & field, const std::string & text);
};
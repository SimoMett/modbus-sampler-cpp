#include <stack>
#include <iostream>
#include "SFMLWorker.h"

const std::string SFMLWorker::WORKER_VERSION = "SFMLWorker build: 1";

SFMLWorker::SFMLWorker(std::shared_ptr<spdlog::logger> logger, std::string window_name, json gui_config, json tags): fps_limit(5), window_name(window_name)
{
    for(const char * field : {"background", "tags"})
    {
        if(gui_config[field].is_null())
            throw std::runtime_error(std::format("Missing '{}' field in config json", field));
    }
    
    if(!gui_config["background"].is_string())
        throw std::runtime_error("Gui 'background' field is not a string");
    
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(gui_config["background"].get<std::string>().c_str());
    patchDocument(doc);
    std::ostringstream prePatchedDoc;
    doc.save(prePatchedDoc);
    this->document = lunasvg::Document::loadFromData(prePatchedDoc.str());
    if(document == nullptr)
        throw std::runtime_error("Could not load svg document");

    if(!gui_config["tags"].is_object())
        throw std::runtime_error("Gui 'tags' is invalid");
    this->gui_tags = gui_config["tags"];
        

    //Parse tag names
    std::unordered_map<addr_t, std::string> *maps[4];
    maps[MbValueType::WORD_TYPE] = &this->words_names;
    maps[MbValueType::DWORD_TYPE] = &this->dwords_names;
    maps[MbValueType::REAL_TYPE] = &this->floats_names;
    maps[MbValueType::COIL_TYPE] = &this->coils_names;

    std::string json_str[4];
    json_str[MbValueType::WORD_TYPE] = "words";
    json_str[MbValueType::DWORD_TYPE] = "dwords";
    json_str[MbValueType::REAL_TYPE] = "floats";
    json_str[MbValueType::COIL_TYPE] = "coils";

    for (auto v : {MbValueType::WORD_TYPE, MbValueType::DWORD_TYPE, MbValueType::REAL_TYPE, MbValueType::COIL_TYPE})
    {
        if (!tags[json_str[v]].is_null())
        {
            for (json s : tags[json_str[v]])
            {
                std::string formatted_tag_name = ConsumerWorker::format_name(s["tag"].get<std::string>());
                maps[v]->insert({s["address"].get<addr_t>(), formatted_tag_name});
            }
        }
    }
}

SFMLWorker::~SFMLWorker(){}

void SFMLWorker::start()
{
    run_thread = std::make_unique<std::thread>(&SFMLWorker::run, this);
}

void SFMLWorker::join()
{
    this->run_thread->join();
}

void SFMLWorker::stop()
{
    this->should_close = true;
}

void SFMLWorker::self_close()
{
    this->should_close = true;
    this->is_running = false;
}

bool SFMLWorker::running()
{
    return is_running;
}

void SFMLWorker::patchDocument(pugi::xml_document &doc)
{
    //DFS
    auto root = doc.root();
    std::stack<pugi::xml_node> grayNodes;
    grayNodes.push(root);

    while(!grayNodes.empty())
    {
        pugi::xml_node node = grayNodes.top();
        grayNodes.pop();

        if(std::string(node.name()) == "switch")
        {
            //Erase 'foreignObject' and pick the next child
            const pugi::xml_node & fo = node.child("foreignObject");
            node.remove_child("foreignObject");
            pugi::xml_node supportedNode = node.first_child();
            node.parent().append_move(supportedNode);
            grayNodes.push(supportedNode); //Parent of 'switch' node doesn't know of its presence
            doc.remove_child(node);
        }
        
        if(!node.children().empty())
        {
            for(auto ch : node.children())
                grayNodes.push(ch);
        }
    }
}

void SFMLWorker::run()
{
    is_running = true;
    try
    {        
        auto bitmap = document->renderToBitmap();
        if(bitmap.isNull())
            throw std::exception();

        bitmap.convertToRGBA();

        sf::Texture window_texture(sf::Vector2u(bitmap.width(), bitmap.height()));

        this->window = sf::RenderWindow(sf::VideoMode(window_texture.getSize()), this->window_name);
        window.setFramerateLimit(fps_limit);

        sf::Sprite window_sprite(window_texture);

        while (window.isOpen())
        {
            while ( const std::optional event = window.pollEvent() )
            {
                if ( event->is<sf::Event::Closed>() )
                    window.close();
            }

            auto bitmap = document->renderToBitmap();
            bitmap.convertToRGBA();
            sf::Image bg_image(sf::Vector2u(bitmap.width(), bitmap.height()), bitmap.data());
            window_texture.update(bg_image);
            window.clear(SFMLWorker::background_color);
            window.draw(window_sprite);
            window.display();
        }
        this->logger->info("SFMLGui worker stopped");
    }
    catch(std::exception & e)
    {
        this->logger->error(e.what());
        this->logger->error("SFMLGui worker stopped due to error");
    }
    this->self_close();
}

inline void SFMLWorker::setItemColor(std::unique_ptr<lunasvg::Document> & doc, const std::string & item_name, const std::string & color)
{
    doc->getElementById(std::string("cell-")+item_name).children()[0].toElement().children()[0].toElement().setAttribute("fill", color);
}

inline void SFMLWorker::setFieldText(std::unique_ptr<lunasvg::Document> & doc, const std::string & field, const std::string & text)
{
    doc->getElementById(std::string("cell-")+field).children()[1].toElement().children()[0].toElement().children()[0].toElement().children()[0].toTextNode().setData(text);
}

void SFMLWorker::dump_samples(){}

void SFMLWorker::push_words(std::vector<AddressValue<uint16_t>> samples, std::chrono::system_clock::time_point time)
{
    for (auto &sample : samples)
    {
        std::string tag_name = words_names[sample.address];
        MbValue val;
        val.word = sample.val;

        if(!gui_tags[tag_name].is_null())
        {
            std::string svg_field = gui_tags[tag_name]["svgField"].get<std::string>();
            if(gui_tags[tag_name]["color"].is_array())
            {
                size_t len = gui_tags[tag_name]["color"].array().size();
                std::string color = gui_tags[tag_name]["color"].array()[val.word % len].get<std::string>();
                setItemColor(this->document, svg_field, color);
            }
            else if(gui_tags[tag_name]["format"].is_string())
            {
                char buffer[16];

                if(gui_tags[tag_name]["rescale"].is_array())
                {
                    std::array<float, 4> v = gui_tags[tag_name]["rescale"].get<std::array<float, 4>>();
                    //float rescaled_val = ((float)val.word - v[0])/((float)v[1] - (float)v[0])*((float)v[3] - (float)v[2])+(float)v[2];
                    float rescaled_val = ((float)val.word - v[0])/(v[1] - v[0])*(v[3] - v[2])+v[2];
                    std::snprintf(buffer, sizeof(buffer), gui_tags[tag_name]["format"].get<std::string>().c_str(), rescaled_val);
                }
                else
                    std::snprintf(buffer, sizeof(buffer), gui_tags[tag_name]["format"].get<std::string>().c_str(), val.word);

                std::string txt = std::string(buffer);
                setFieldText(this->document, svg_field, txt);
            }
        }
    }
}

void SFMLWorker::push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point){}
void SFMLWorker::push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point){}
void SFMLWorker::push_coils(std::vector<AddressValue<bool>>, std::chrono::system_clock::time_point){}
void SFMLWorker::push_bits(std::vector<BitAddressValue>, std::chrono::system_clock::time_point){}
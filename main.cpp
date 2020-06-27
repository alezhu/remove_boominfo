#include <sstream>
#include <filesystem>
#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_print.hpp>
#include "libzippp.h"
#include <string>
#include <iostream>

using namespace libzippp;
using namespace rapidxml;


static const std::string DOCX_WORD_DOCUMENT_PATH{"word/document.xml"};
static const char *const DOCX_XML_TAG_DOCUMENT = "w:document";
static const char *const DOCX_XML_TAG_BODY = "w:body";
static const char *const DOCX_XML_TAG_P = "w:p";
static const char *const DOCX_XML_TAG_HYPERLINK = "w:hyperlink";
static const char *const DOCX_XML_TAG_ROW = "w:r";
static const char *const DOCX_XML_TAG_TEXT = "w:t";
static const char *const BOOMINFO_URL = "https://boominfo.ru";

//void process_file(std::filesystem::path path);
//
//void process_dir(std::filesystem::path path);
//
//template<typename Ch>
//bool is_ads(xml_node<Ch> *pNode);

template<typename Ch>
bool is_ads(xml_node<Ch> *pNode) {
    auto link = pNode->first_node(DOCX_XML_TAG_HYPERLINK);
    if (nullptr == link) return false;

    auto r = link->first_node(DOCX_XML_TAG_ROW);
    if (nullptr == r) return false;

    auto t = r->first_node(DOCX_XML_TAG_TEXT);
    if (nullptr == t) return false;

    if (strcmp(t->value(), BOOMINFO_URL) != 0) return false;

    return true;
}

void process_file(const std::filesystem::path &path) {
    std::filesystem::path back_path = path;
    back_path += ".bak";
    if (std::filesystem::exists(back_path)) {
        if (std::filesystem::file_size(path) != std::filesystem::file_size(back_path)) {
            //restore
            std::filesystem::remove(path);
            std::filesystem::copy(back_path, path);
        }
    };
    auto utf8path = path.generic_u8string();
    auto utf8_buffer = utf8path.c_str();
    std::string path_str{reinterpret_cast<const char *>(utf8_buffer)};
    ZipArchive zip(path_str);
    if (!zip.open(ZipArchive::WRITE)) return;
    auto doc_xml_file = zip.getEntry(DOCX_WORD_DOCUMENT_PATH);
    bool modified = false;
    if (!doc_xml_file.isNull()) {
        auto doc_xml_text = doc_xml_file.readAsText();
        xml_document doc_xml;
        doc_xml.parse<0>(doc_xml_text.data());
        auto document = doc_xml.first_node(DOCX_XML_TAG_DOCUMENT);
        if (nullptr != document) {
            auto body = document->first_node(DOCX_XML_TAG_BODY);
            if (nullptr != body) {
                auto p = body->first_node(DOCX_XML_TAG_P);
                while (nullptr != p) {
                    auto next = p->next_sibling(DOCX_XML_TAG_P);
                    if (is_ads(p)) {
                        body->remove_node(p);
                        modified = true;
                    }
                    p = next;
                }
                if (modified) {
                    std::ostringstream output;
//                    output << doc_xml;
                    std::ostream_iterator<char> iter(output);
                    print(iter, doc_xml, print_no_indenting);
                    auto file = output.str();
                    auto length = file.length();
                    zip.addData(doc_xml_file.getName(), file.data(), length);

                    if (!std::filesystem::exists(back_path)) {
                        std::filesystem::copy(path, back_path);
                    }
                    zip.close();

                    std::cout << "File changed: " << path << std::endl;
                }
            }
        }
    }
    zip.close();
}


void process_dir(const std::filesystem::path &path) {
    std::filesystem::recursive_directory_iterator begin(path);
    std::filesystem::recursive_directory_iterator end;
    for (auto it = begin; it != end; ++it) {
        if (it->is_regular_file()) {
            auto item_path = it->path();
            if (item_path.extension() == ".docx") {
                process_file(item_path);
            }
        }
    }

}

int main(int argc, char *argv[]) {

    std::filesystem::path path;
    if (1 == argc) {
        path = std::filesystem::current_path();
    } else {
        path = std::string(argv[1]);;
    }

    if (std::filesystem::is_directory(path)) {
        process_dir(path);
    } else {
        process_file(path);
    }

    return 0;
}
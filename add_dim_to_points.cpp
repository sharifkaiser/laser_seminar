/*H**********************************************************************
* FILENAME :        add_dim_to_points.cpp
*
* DESCRIPTION :
*       Calculates and adds dimension Height|Width in the beginning of input text file containing svg image points
*

How to call: 
    1  ./add_dim_to_points

AUTHOR :    A K M Sharif Kaiser(SK)        START DATE : 09 Mar 2021

CHANGES :
REF NO  VERSION DATE    WHO     DETAIL
* 02    25MAR2021       SK      Dimension change from absolute height|width to window relative height|width

*H*/


#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <regex>
#include<string>
#include<cstdio>
namespace fs = std::filesystem;

// Global variables
std::vector<std::string> txt_files;
std::vector<double> x_coordinates, y_coordinates;
double x_min, x_max, y_min, y_max, height, width;
std::string line, x, y, file_content="", first_line ="", file_arg = "";
std::size_t offset = 0;
bool is_processing_reqd = true;    // if the file should be processed to add dimensions
int pos;
const std::regex txt_regex("^.*\\.\\(?txt$|TXT$\\)[^.]+$");  // text file regex, // for escape


void process_file(std::string & file_name){
    std::ifstream current_file;
    current_file.open(file_name, std::ios::in);
    if (current_file.is_open())
    {
        // read file into file_content
        current_file.seekg(0, std::ios::end);   
        file_content.reserve(current_file.tellg());
        current_file.seekg(0, std::ios::beg);

        while (std::getline(current_file, line))
        {
            file_content = file_content + line + "\n";

            if (line.find(",") != std::string::npos)    // npos means not found
            {
                pos = line.find(",");
                x = line.substr(0, pos);   // x coordinate
                y = line.substr(pos + 1, line.length());   // y cordinate

                try{
                    x_coordinates.push_back(std::stod (x, &offset));
                    y_coordinates.push_back(std::stod (y, &offset));
                }
                catch (const std::invalid_argument&) {
                    std::cout << "Height|Width was not added to " + file_name + " because of invalid point entries" << std::endl;
                    is_processing_reqd = false;
                    break;
                }
                catch (const std::out_of_range&) {
                    std::cout << "Height|Width was not added to " + file_name + " because of invalid point entries" << std::endl;
                    is_processing_reqd = false;
                    break;
                }
                
            }
            else if (line.compare("#") == 0)    // end of file
            {
                break; //exit from while loop
            }
            else if (line.find("|") != std::string::npos)
            {
                std::cout << "Height|Width was not added to " + file_name + " because it already has dimensions" << std::endl;
                // encountered | which means the text file already has dimensions, so do not process the file
                is_processing_reqd = false;
                break;
            }
            else
            {
                std::cout << "Height|Width was not added to " + file_name + " because it is invalid" << std::endl;
                /* this happens only if it is invalid file, so do not process */
                is_processing_reqd = false;
                break;
            } 
        }

        if (is_processing_reqd)
        {
            // image window always starts from (0,0), so only max coordinate is relevant for dimension
            width = *std::max_element(x_coordinates.begin(), x_coordinates.end());
            height = *std::max_element(y_coordinates.begin(), y_coordinates.end());

            first_line = std::to_string((int)height) + "|" + std::to_string((int)width); // First line height|width

            current_file.close();        // close file

            // Remove the existing file
            if(std::filesystem::remove(file_name)){
                std::cout << file_name << " has been removed" << std::endl;
            }

            // create new file with the same name
            std::ofstream new_file;
            new_file.open(file_name, std::ios::out);
            if (!new_file) {
                std::cout << "File not created!" << std::endl;
            }
            else {
                std::cout << file_name + " has been added successfully with " + first_line + " in the first line!" << std::endl;
                new_file << first_line + "\n" + file_content ;   // First line + \n + prev content
                new_file.close();
            }
        }
    }
}

int main(int argc, char* argv[])
{

    if(argc > 1){
        file_arg = argv[1];
        if (file_arg.rfind("./", 0) != 0) {
            file_arg = "./" + file_arg;     // add directory in the beginning
        }
    }

    if (file_arg.length() > 0)
    {
        /* program has a file argument, just process this file */
        if (std::filesystem::exists(file_arg))
        {
            process_file(file_arg);
        }
        else
        {
            std::cout << file_arg + "does not exist in the current directory. No dimension was added to file." << std::endl;
        }
        
    }
    else
    {
        /* batch process: process all text files in current dir */
        std::string path = "./";        // current dir
        for (const auto & entry : fs::directory_iterator(path)){
            if (std::regex_match(entry.path().string(), txt_regex)) {
                std::cout << entry.path().string() << std::endl;
                txt_files.push_back(entry.path().string());
            }
        }

        for (int i = 0; i < txt_files.size(); i++){
            // init variables
            first_line = "";
            file_content = "";
            is_processing_reqd = true;
            process_file(txt_files.at(i));
        }
    }
}

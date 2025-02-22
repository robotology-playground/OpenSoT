/*
 * Copyright (C) 2017 IIT-ADVR
 * Author: Giuseppe Rigano
 * email:  giuseppe.rigano@iit.it
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
*/


#ifndef __SO_LIB_H__
#define __SO_LIB_H__

#include <map>
#include <string>
#include <iostream>

#include <dlfcn.h>
#include <OpenSoT/utils/RtLog.hpp>

#define REGISTER_SO_LIB_(class_name, base_class) \
extern "C" base_class* create_instance() \
{ \
  return new  class_name(); \
}\
\
extern "C" void destroy_instance( base_class* instance ) \
{ \
  delete instance; \
}\



using XBot::Logger;

namespace
{


class SoLib
{

public:

    template <class T>
    static std::shared_ptr<T> getFactory ( const std::string& path_to_so, const std::string& lib_name ) {
        char *error;
        void* lib_handle;
        lib_handle = dlopen ( path_to_so.c_str(), RTLD_NOW );
        if ( !lib_handle ) {
            XBot::Logger::error() << lib_name <<" so library NOT found! \n" << dlerror() << XBot::Logger::endl();
        } else {
            Logger::success() << lib_name << " so library found! " << Logger::endl();
            handles[lib_name] = lib_handle;

            T* ( *create ) ();
            create = ( T* ( * ) () ) dlsym ( lib_handle, "create_instance" );
            if ( ( error = dlerror() ) != NULL ) {
                XBot::Logger::error() << " in loading library " << lib_name << ": \n" << error << XBot::Logger::endl();
                return nullptr;
            }

            T* instance = ( T* ) create();
            if ( instance != nullptr ) {
                return std::shared_ptr<T> ( instance );
            }

            XBot::Logger::error() << " in loading library " << lib_name << ": obtained pointer is null" << XBot::Logger::endl();
        }

        return nullptr;

    }

    static void unloadLib ( const std::string& file_name ) {
        dlclose ( handles[file_name] );
        Logger::info() << file_name <<" so library unloaded! " << Logger::endl();
    }

    template <class T, typename... Args>
    static std::shared_ptr<T> getFactoryWithArgs ( const std::string& path_to_so, const std::string& lib_name, Args... args ) {
        char *error;
        void* lib_handle;
        lib_handle = dlopen ( path_to_so.c_str(), RTLD_NOW );
        if ( !lib_handle ) {
            XBot::Logger::error() << lib_name <<" so library NOT found! \n" << dlerror() << XBot::Logger::endl();
        } else {
            Logger::success() << lib_name << " so library found! " << Logger::endl();
            handles[lib_name] = lib_handle;

            T* ( *create ) ( Args... args );
            create = ( T* ( * ) ( Args... args ) ) dlsym ( lib_handle, "create_instance" );
            if ( ( error = dlerror() ) != NULL ) {
                XBot::Logger::error() << " in loading library " << lib_name << ": \n" << error << XBot::Logger::endl();
                return nullptr;
            }

            T* instance = ( T* ) create ( args... );
            if ( instance != nullptr ) {
                return std::shared_ptr<T> ( instance );
            }

            XBot::Logger::error() << " in loading library " << lib_name << ": obtained pointer is null" << XBot::Logger::endl();
        }

        return nullptr;

    }

private:

    SoLib() = delete;

    static std::map<std::string, void*> handles;

    static bool computeAbsolutePath ( const std::string& input_path,
                                      const std::string& middle_path,
                                      std::string& absolute_path ) {
        // if not an absolute path
        if ( ! ( input_path.at ( 0 ) == '/' ) ) {
            // if you are working with the Robotology Superbuild
            const char* env_p = std::getenv ( "XBOT_ROOT" );
            // check the env, otherwise error
            if ( env_p ) {
                std::string current_path ( env_p );
                // default relative path when working with the superbuild
                current_path += middle_path;
                current_path += input_path;
                absolute_path = current_path;
                return true;
            } else {
                std::cerr << "ERROR in " << __func__ << " : the input path  " << input_path << " is neither an absolute path nor related with the robotology superbuild. Download it!" << std::endl;
                return false;
            }
        }
        // already an absolute path
        absolute_path = input_path;
        return true;
    }


};

std::map<std::string, void*> SoLib::handles;

}

#endif

/**
 * @file modules.hxx
 *
 * @brief Includes all available modules for the application.
 */

#ifndef CLUEAPI_MODULES_HXX
#define CLUEAPI_MODULES_HXX

#ifdef CLUEAPI_USE_LOGGING_MODULE
#include "logging/logging.hxx"
#endif // CLUEAPI_USE_LOGGING_MODULE

#ifdef CLUEAPI_USE_DOTENV_MODULE
#include "dotenv/dotenv.hxx"
#else
// ...
#endif // CLUEAPI_USE_DOTENV_MODULE

/**
 * @namespace clueapi::modules
 *
 * @brief Namespace for all available modules.
 */
namespace clueapi::modules {
    // ...
}

#endif // CLUEAPI_MODULES_HXX
#pragma once

#include "cf_utility.hpp"
#include <Security/CodeSigning.h>
#include <optional>
#include <pqrs/cf_string.hpp>
#include <string>

namespace krbn {
class codesign {
public:
  static std::optional<std::string> get_common_name_of_process(pid_t pid) {
    std::optional<std::string> common_name;

    if (auto attributes = cf_utility::create_cfmutabledictionary()) {
      if (auto pid_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &pid)) {
        CFDictionarySetValue(attributes, kSecGuestAttributePid, pid_number);

        SecCodeRef guest;
        if (SecCodeCopyGuestWithAttributes(nullptr, attributes, kSecCSDefaultFlags, &guest) == errSecSuccess) {
          CFDictionaryRef information;
          if (SecCodeCopySigningInformation(guest, kSecCSSigningInformation, &information) == errSecSuccess) {
            if (auto certificates = static_cast<CFArrayRef>(CFDictionaryGetValue(information, kSecCodeInfoCertificates))) {
              if (CFArrayGetCount(certificates) > 0) {
                auto certificate = cf_utility::get_value<SecCertificateRef>(certificates, 0);
                CFStringRef common_name_string;
                if (SecCertificateCopyCommonName(certificate, &common_name_string) == errSecSuccess) {
                  common_name = pqrs::make_string(common_name_string);
                  CFRelease(common_name_string);
                }
              }
            }

            CFRelease(information);
          }

          CFRelease(guest);
        }

        CFRelease(pid_number);
      }

      CFRelease(attributes);
    }

    return common_name;
  }
};
} // namespace krbn

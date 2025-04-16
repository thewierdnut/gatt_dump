#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <set>

struct _GDBusProxy;
struct _GVariantIter;
struct _GVariant;

namespace asha
{

// Abstraction of gatt descriptor using dbus
class Descriptor final
{
public:
   Descriptor() {}
   Descriptor(const std::string& path, struct _GVariantIter* properties);
   ~Descriptor();

   Descriptor& operator=(const Descriptor& o);

   const std::string& Path() const { return m_path; }
   const std::string& UUID() const { return m_uuid; }
   const std::string& Characteristic() const { return m_char_path; }

   // Read the given descriptor.
   std::vector<uint8_t> Read();
   // Write to the given descriptor.
   bool Write(const std::vector<uint8_t>& bytes);
   
   operator bool() const { return !m_uuid.empty(); }

protected:
   void CreateProxyIfNotAlreadyCreated() noexcept;

   std::shared_ptr<_GVariant> Call(const char* fname, const std::shared_ptr<_GVariant>& args = nullptr) noexcept;

private:
   std::shared_ptr<_GDBusProxy> m_desc;
   
   std::string m_uuid;
   std::string m_path;
   std::string m_char_path;
};

}
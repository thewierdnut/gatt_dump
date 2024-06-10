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

static constexpr char CHARACTERISTIC_INTERFACE[] = "org.bluez.GattCharacteristic1";


// Abstraction of bluez interface using dbus
class Characteristic final
{
public:
   Characteristic() {}
   Characteristic(const std::string& path, struct _GVariantIter* properties);
   ~Characteristic();

   Characteristic& operator=(const Characteristic& o);

   const std::string& Path() const { return m_path; }
   const std::string& UUID() const { return m_uuid; }
   const std::set<std::string>& Flags() const { return m_flags; }
   const std::string& Service() const { return m_service_path; }

   // Read the given Gatt characteristic.
   std::vector<uint8_t> Read();
   // Write to the given Gatt characteristic.
   bool Write(const std::vector<uint8_t>& bytes);
   // Command the given Gatt characteristic.
   bool Command(const std::vector<uint8_t>& bytes);
   // When the given Gatt characteristic is notified, call the given function.
   bool Notify(std::function<void(const std::vector<uint8_t>&)> fn);
   void StopNotify();

   operator bool() const { return !m_uuid.empty(); }

protected:
   void CreateProxyIfNotAlreadyCreated() noexcept;

   std::shared_ptr<_GVariant> Call(const char* fname, const std::shared_ptr<_GVariant>& args = nullptr) noexcept;

private:
   std::shared_ptr<_GDBusProxy> m_char;
   
   std::string m_uuid;
   std::string m_path;
   std::string m_service_path;

   std::set<std::string> m_flags;

   unsigned long m_notify_handler_id = -1;
   std::function<void(const std::vector<uint8_t>&)> m_notify_callback;
};

}
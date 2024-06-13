#include "src/Bluetooth.hh"

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/glib-unix.h>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>


const std::set<std::string> bad_read_uuids = {
   "30e69638-3752-4feb-a3aa-3226bcd05ace",    // It disconnects when I try to read this, but notification subscriptions succeed.
   "2bdcaebe-8746-45df-a841-96b840980fb8",    // Disconnects on read.
   "2bdcaebe-8746-45df-a841-96b840980fb7",    // Disconnects on read.
};

template <typename T>
std::string join(const std::string& s, const T& list)
{
   if (list.empty()) return "";
   auto it = list.begin();
   std::stringstream ss;
   ss << *it;
   for (++it; it != list.end(); ++it)
      ss << s << *it;
   return ss.str();
}

class GattDump
{
public:
   GattDump():
      m_b(
         [this](const asha::Bluetooth::BluezDevice& d) { OnAddDevice(d); },
         [this](const std::string& p) { OnRemoveDevice(p); }
      )
   {
      
   }

protected:
   static std::string HexDump(const std::vector<uint8_t>& bytes)
   {
      std::stringstream ss;
      bool first = true;
      for (uint8_t b: bytes)
      {
         if (!first) ss << ' ';
         ss << std::hex << std::setfill('0') << std::setw(2) << (unsigned)b;
         first = false;
      }
      return ss.str();
   }

   static std::string Printable(const std::vector<uint8_t>& bytes)
   {
      std::string ret;
      for (uint8_t c: bytes)
      {
         switch (c)
         {
         case '\0': ret += "\\0"; break;
         case '\a': ret += "\\a"; break;
         case '\b': ret += "\\b"; break;
         case '\t': ret += "\\t"; break;
         case '\n': ret += "\\n"; break;
         case '\v': ret += "\\v"; break;
         case '\f': ret += "\\f"; break;
         case '\r': ret += "\\r"; break;
         default:
            if (c >= 32 && c < 127)
               ret += (char)c;
            else
            {
               std::stringstream ss;
               ss << std::setfill('0') << std::setw(3) << std::oct << (unsigned)c;
               ret += "\\" + ss.str();
            }
         }
      }
      return ret;
   }


   void OnAddDevice(const asha::Bluetooth::BluezDevice& d)
   {
      static bool once = false;
      if (once) return;
      once = true;
      std::cout << d.name << " with " << d.services.size() << " services\n";

      for (auto& kv: d.services)
      {
         std::cout << "   " << kv.second.uuid << " " << kv.second.path << '\n';
         for (auto& read_only_c: kv.second.characteristics)
         {
            // Copy this so that we can change its state (for notifications)
            m_characteristics[read_only_c.Path()].reset(new asha::Characteristic(read_only_c));
            auto& c = *m_characteristics[read_only_c.Path()];
            std::cout << "      " << c.UUID() << " " << c.Path() << " [" << join(", ", c.Flags()) << "] ";
            if (c.Flags().count("notify"))
            {
               std::cout << "[subscribed] ";

               c.Notify([=](const std::vector<uint8_t> &v) {
                  std::cout << "Notify: " << c.UUID() << " " << c.Path() << " " << HexDump(v) << '\n';
               });
            }
            if (c.Flags().count("read"))
            {
               if (bad_read_uuids.count(c.UUID()))
                  std::cout << " <not read>";
               else
               {
                  auto value = c.Read();
                  std::cout << HexDump(value) << " \"" << Printable(value) << "\"";
               }
            }
            std::cout << "\n";
         }
      }
   }
   void OnRemoveDevice(const std::string& path)
   {
      
   }

protected:
   


private:
   std::map<std::string, std::shared_ptr<asha::Characteristic>> m_characteristics;

   asha::Bluetooth m_b; // needs to be last
};


int main()
{
   setenv("G_MESSAGES_DEBUG", "all", false);
   GattDump c;


   std::shared_ptr<GMainLoop> loop(g_main_loop_new(nullptr, true), g_main_loop_unref);
   auto quitter = g_unix_signal_add(SIGINT, [](void* ml) {
      g_main_loop_quit((GMainLoop*)ml);
      return (int)G_SOURCE_CONTINUE;
   }, loop.get());

   g_main_loop_run(loop.get());
   g_source_remove(quitter);

   std::cout << "Stopping...\n";

   return 0;
}
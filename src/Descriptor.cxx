#include "Descriptor.hh"
#include "GVariantDump.hh"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <sstream>

#include <gio/gio.h>

#include "GVariantDump.hh"

using namespace asha;


Descriptor::Descriptor(const std::string& path, GVariantIter* properties):
   m_path(path)
{
   gchar* key{};
   GVariant* value{};
   while (g_variant_iter_loop(properties, "{sv}", &key, &value))
   {
      if (g_str_equal("UUID", key))
         m_uuid = g_variant_get_string(value, nullptr);
      else if (g_str_equal("Characteristic", key))
         m_char_path = g_variant_get_string(value, nullptr);
      // else if (g_str_equal("Value", key))
      //    g_info("Descriptor::Value is %s", GVariantDump(value).c_str());
   }
}

Descriptor::~Descriptor()
{
}

Descriptor& Descriptor::operator=(const Descriptor& o)
{
   m_desc.reset();
   m_uuid = o.m_uuid;
   m_path = o.m_path;
   m_char_path = o.m_char_path;
   return *this;
}


std::vector<uint8_t> Descriptor::Read()
{
   // Args needs to be a tuple containing dict options. (dbus dicts are arrays
   // of key/value pairs).
   GVariantBuilder b = G_VARIANT_BUILDER_INIT(G_VARIANT_TYPE("a{sv}"));
   g_variant_builder_add(&b, "{sv}", "offset", g_variant_new_uint16(0));
   
   GVariantBuilder ab = G_VARIANT_BUILDER_INIT(G_VARIANT_TYPE("(a{sv})"));
   g_variant_builder_add_value(&ab, g_variant_builder_end(&b));

   std::shared_ptr<GVariant> args(g_variant_builder_end(&ab), g_variant_unref);
   
   // If you want to know why this is here, search google for "glib floating
   // reference", and be prepared to get very, very angry.
   g_variant_ref_sink(args.get());

   GError* e = nullptr;
   auto result = Call("ReadValue", args);

   if (!result)
      return {};

   if (!g_variant_check_format_string(result.get(), "(ay)", false))
   {
      g_warning("Incorrect type signature when reading %s: %s", m_path.c_str(), g_variant_get_type_string(result.get()));
      return {};
   }

   gsize length = 0;
   std::shared_ptr<GVariant> ay(g_variant_get_child_value(result.get(), 0), g_variant_unref);
   guint8* data = (guint8*)g_variant_get_fixed_array(ay.get(), &length, sizeof(guint8));


   std::vector<uint8_t> ret(data, data + length);

   ay.reset();
   result.reset();
   args.reset();


   return ret;
}

bool Descriptor::Write(const std::vector<uint8_t>& bytes)
{
   // Args is a tuple containing a byte aray and the dict options.
   GVariantBuilder b = G_VARIANT_BUILDER_INIT(G_VARIANT_TYPE("a{sv}"));
   g_variant_builder_add(&b, "{sv}", "offset", g_variant_new_uint16(0));
   
   GVariantBuilder ab = G_VARIANT_BUILDER_INIT(G_VARIANT_TYPE("(aya{sv})"));
   g_variant_builder_add_value(&ab, g_variant_new_fixed_array(G_VARIANT_TYPE("y"), bytes.data(), bytes.size(), sizeof(gint8)));
   g_variant_builder_add_value(&ab, g_variant_builder_end(&b));

   std::shared_ptr<GVariant> args(g_variant_builder_end(&ab), g_variant_unref);
   
   // If you want to know why this is here, search google for "glib floating
   // reference", and be prepared to get very, very angry.
   g_variant_ref_sink(args.get());

   GError* e = nullptr;
   auto result = Call("WriteValue", args);

   if (!result)
      return false;

   if (!g_variant_check_format_string(result.get(), "()", false))
   {
      g_warning("Incorrect type signature when reading %s: %s", m_path.c_str(), g_variant_get_type_string(result.get()));
      return false;
   }
   return true;
}


void Descriptor::CreateProxyIfNotAlreadyCreated() noexcept
{
   // What a great function name!
   if (m_desc) return;

   GError* err = nullptr;
   m_desc.reset(g_dbus_proxy_new_for_bus_sync(
      G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      nullptr,
      "org.bluez",
      m_path.c_str(),
      "org.bluez.GattDescriptor1",
      nullptr,
      &err
   ), g_object_unref);

   if (err)
   {
      g_error("Error getting dbus %s proxy: %s", "org.bluez.GattDescriptor1", err->message);
      g_error_free(err);
      m_desc.reset();
      return;
   }
}


std::shared_ptr<_GVariant> Descriptor::Call(const char* fname, const std::shared_ptr<_GVariant>& args) noexcept
{
   CreateProxyIfNotAlreadyCreated();

   if (m_desc)
   {
      GError* e = nullptr;
      // Cannot directly capture result into a shared_ptr because the shared_ptr
      // will happily delete a null pointer, which g_variant_unref does not like.
      GVariant* result = g_dbus_proxy_call_sync(m_desc.get(),
         fname,
         args.get(),
         G_DBUS_CALL_FLAGS_NONE,
         -1,
         nullptr,
         &e
      );
      if (e)
      {
         g_info("Error calling %s: %s", fname, e->message);
         g_error_free(e);
         return nullptr;
      }
      if (result)
      {
         // Convert from floating reference to a full reference.
         // g_variant_ref_sink(result);
         return std::shared_ptr<GVariant>(result, g_variant_unref);
      }
      else
      {
         g_warning("Null result when calling %s", fname);
      }
   }

   return nullptr;
}

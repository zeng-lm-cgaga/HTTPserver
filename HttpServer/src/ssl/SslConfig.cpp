#include "../../include/ssl/SslConfig.h"

namespace ssl
{
SslConfig::SslConfig()
    : version_(SSLVersion::TLS_1_2)
    , cipherList_("HIGH:!aNULL:!MD5")
    , verifyClient_(false)
    , verifyDepth_(4)
    , sessionTimeout_(300)
    , sessionCacheSize_(20480L)
{
}

};
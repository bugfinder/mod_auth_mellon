# mod_auth_mellon

mod_auth_mellon is an authentication module for Apache. It authenticates
the user against a SAML 2.0 IdP, and grants access to directories
depending on attributes received from the IdP.

This module, under the `latchset` organization, is under active maintenance.
It was forked from the original Uninett repository when Uninett chose to
retire their version.

## Dependencies

mod_auth_mellon has the following dependencies:
 * pkg-config
 * Apache (>=2.0)
 * OpenSSL
 * libcURL
 * lasso (>=2.4)

You will also require development headers and tools for all of the
dependencies.

If OpenSSL or lasso are installed in a "strange" directory, then you may
have to specify the directory containing "lasso.pc" and/or "openssl.pc" in
the PKG_CONFIG_PATH environment variable. For example, if OpenSSL is
installed in /usr/local/openssl (with openssl.pc in
/usr/local/openssl/lib/pkgconfig/) and lasso is installed in /opt/lasso
(lasso.pc in /opt/lasso/lib/pkgconfig/), then you can set PKG_CONFIG_PATH
before running configure like this:
```
PKG_CONFIG_PATH=/usr/local/openssl/lib/pkgconfig:/opt/lasso/lib/pkgconfig
export PKG_CONFIG_PATH
```
If Apache is installed in a "strange" directory, then you may have to
specify the path to apxs2 using the `--with-apxs2=/full/path/to/apxs2`
option to configure. If, for example, Apache is installed in /opt/apache,
with apxs2 in /opt/apache/bin, then you run
```
./configure --with-apxs2=/opt/apache2/bin/apxs2
```
Note that, depending on your distribution, apxs2 may be named apxs.


## Installing mod_auth_mellon

mod_auth_mellon uses autoconf, and can be installed by running the
following commands after extracting the release tarball:
```
./configure
make
make install
```

Are you not using the release tarball but working straight from a
git checkout, you need to run `./autogen.sh` first.

## Configuring mod_auth_mellon

Here we are going to assume that your web server's hostname is
'example.com', and that the directory you are going to protect is
'https://example.com/secret/'. We are also going to assume that you have
configured your web site to use SSL.

You need to edit the configuration file for your web server. Depending on
your distribution, it may be named '/etc/apache/httpd.conf' or something
different.

You need to add a LoadModule directive for mod_auth_mellon. This will
look similar to this:
```
LoadModule auth_mellon_module /usr/lib/apache2/modules/mod_auth_mellon.so
```
To find the full path to mod_auth_mellon.so, you may run:
```
apxs2 -q LIBEXECDIR
```
This will print the path where Apache stores modules. mod_auth_mellon.so
will be stored in that directory.

You will also need to make sure that Apache's authn_core module is also
enabled. Most likely you also want authz_user to be enabled.

After you have added the LoadModule directive, you must add configuration
for mod_auth_mellon. The following is an example configuration:

```ApacheConf
###########################################################################
# Global configuration for mod_auth_mellon. This configuration is shared by
# every virtual server and location in this instance of apache.
###########################################################################

# MellonSoCache The Apache Shared Object Cache provider and it's provider
# specific (optional) configuration parameters. Mellon will use this
# Apache Shared Object Cache provider to store it's persistent session data
# as well as other Mellon internal persisted data.
#
# The directive has the form name:args where the provider name is
# separated by the provider arguments by a colon. If no provider
# arguments are required the form is just the provider name.
# 
# The available Apache Shared Object Cache providers are documented in
# the Apache Shared Object Cache documentation. After selecting a
# provider you will need to refer to that providers documentation for
# any provider specific configuration parameters you might need to
# pass in the args portion of the directive. If the MellonSoCache
# directive is not defined it defaults to Apache's default shared
# object cache provider. Mellon has no control over what Apache's
# default provider is but currently it is "shmcb".
#
# As an example, to use memcache running locally on port 11211 the
# directive would be:
#
# MellonSoCache memcache:localhost:11211
#
# where "memcache" is the provider name and it's optional are
# "localhost:11211"

# MellonSoCacheSessionStateEntrySize
# The maximum number of octets in a socache session state entry.
# Default: 65536

# MellonCacheSize - DEPRECATED
# Retained only to prevent module load errors if specified.  If
# specified it has no effect. This config item will be removed in a
# future release.


# MellonCacheEntrySize - DEPRECATED
# Retained only to prevent module load errors if specified.  If
# specified it has no effect. This config item will be removed in a
# future release.

# MellonLockFile - DEPRECATED
# Retained only to prevent module load errors if specified.  If
# specified it has no effect. This config item will be removed in a
# future release.

# MellonPostDirectory is the full path of a directory where POST requests
# are saved during authentication. This directory must writable by the
# Apache user. It should not be writable (or readable) by other users.
# Default: None
# Example: MellonPostDirectory "/var/cache/mod_auth_mellon_postdata"

# MellonPostTTL is the delay in seconds before a saved POST request can
# be flushed.
# Default: MellonPostTTL 900 (15 mn)
MellonPostTTL 900

# MellonPostSize is the maximum size for saved POST requests
# Default: MellonPostSize 1048576 (1 MB)
MellonPostSize 1048576

# MellonPostCount is the maximum amount of saved POST requests
# Default: MellonPostCount 100
MellonPostCount 100

# MellonDiagnosticsFile If Mellon was built with diagnostic capability
# then diagnostic is written here, it may be either a filename or a pipe.
# If it's a filename then the resulting path is  relative to the ServerRoot.
# If the value is preceeded by the pipe character "|" it should be followed
# by a path to a program to receive the log information on its standard input.
# This is a server context directive, hence it may be specified in the
# main server config area or within a <VirtualHost> directive.
# Default: logs/mellon_diagnostics
MellonDiagnosticsFile logs/mellon_diagnostics

# MellonDiagnosticsEnable If Mellon was built with diagnostic capability
# then this is a list of words controlling diagnostic output.
# Currently only On and Off are supported.
# This is a server context directive, hence it may be specified in the
# main server config area or within a <VirtualHost> directive.
# Default: Off
MellonDiagnosticsEnable Off

###########################################################################
# End of global configuration for mod_auth_mellon.
###########################################################################


# This defines a directory where mod_auth_mellon should do access control.
<Location /secret>

        # These are standard Apache apache configuration directives.
        # You must have enabled Apache's authn_core and authz_user modules.
        # See http://httpd.apache.org/docs/2.4/mod/mod_authn_core.html and
        # http://httpd.apache.org/docs/2.4/mod/mod_authz_user.html
        # about them.
        Require valid-user
        AuthType "Mellon"


        # MellonEnable is used to enable auth_mellon on a location.
        # It has three possible values: "off", "info" and "auth".
        # They have the following meanings:
        #   "off":  mod_auth_mellon will not do anything in this location.
        #           This is the default state.
        #   "info": If the user is authorized to access the resource, then
        #           we will populate the environment with information about
        #           the user. If the user isn't authorized, then we won't
        #           populate the environment, but we won't deny the user
        #           access either.
        #
        #           You can also use this to set up the Mellon SSO paramaters
        #           transparently at the top level of your site, and then use
        #           "auth" to protect individual paths elsewhere in the site.
        #   "auth": We will populate the environment with information about
        #           the user if he is authorized. If he is authenticated
        #           (logged in), but not authorized (according to the
        #           MellonRequire and MellonCond directives, then we will 
        #           return a 403 Forbidden error. If he isn't authenticated
        #           then we will redirect him to the login page of the IdP.
        #
        #           There is a special handling of AJAX requests, that are
        #           identified by the "X-Requested-With: XMLHttpRequest" HTTP
        #           header. Since no user interaction can happen there,
        #           we always fail unauthenticated (not logged in) requests
        #           with a 403 Forbidden error without redirecting to the IdP.
        #
        # Default: MellonEnable "off"
        MellonEnable "auth"

        # MellonDecoder is an obsolete option which is a no-op but is
        # still accepted for backwards compatibility.

        # MellonVariable is used to select the name of the cookie which
        # mod_auth_mellon should use to remember the session id. If you
        # want to have different sites running on the same host, then
        # you will have to choose a different name for the cookie for each
        # site.
        # Default: "cookie"
        MellonVariable "cookie"

        # Whether the cookie set by auth_mellon should have HttpOnly and
        # secure flags set. Once "On" - both flags will be set. Values 
        # "httponly" or "secure" will respectively set only one flag.
        # Default: Off
        MellonSecureCookie On

        # MellonCookieDomain allows to specify of the cookie which auth_mellon
        # will set.
        # Default: the domain for the received request (the Host: header if
        # present, of the ServerName of the VirtualHost declaration, or if
        # absent a reverse resolution on the local IP)
        # MellonCookieDomain example.com

        # MellonCookiePath is the path of the cookie which auth_mellon will set.
        # Default: /
        MellonCookiePath /

        # MellonCookieSameSite allows control over the SameSite value used
        # for the authentication cookie.
        # The setting accepts values of "Strict", "Lax", or "None".
        # When using none, you should set "MellonSecureCookie On" to prevent
        # compatibility issues with newer browsers.
        # If not set, the SameSite attribute is not set on the cookie. In newer
        # browsers, this may cause SameSite to default to "Lax".
        # Note: Regardless of the value set here a fixed SameSite value of
        # None is used for the cookie test. The cookie test, which is performed
        # with a static value, should detect whether the user's client accepts
        # cookies or not before the auth_mellon session is established and thus
        # avoid a redirect loop. Using a fixed SameSite value of None ensures
        # that the cookie with the static value does not get lost in the
        # HTTP-POST binding request issued by the autosubmit form returned by
        # the IDP.
        # Default: not set
        # MellonCookieSameSite lax

        # Some browsers will reject cookies if SameSite is specified.
        # The MELLON_DISABLE_SAMESITE environment variable suppresses
        # setting of SameSite cookies. You can use the following directives
        # to set it.
        # BrowserMatch "\(iP.+; CPU .*OS 12[_\d]*.*\) AppleWebKit\/" MELLON_DISABLE_SAMESITE=1
        # BrowserMatch "\(Macintosh; Intel Mac OS X 10_14_\d\) AppleWebKit\/[\.\d]+ \(KHTML, like Gecko\)$" MELLON_DISABLE_SAMESITE=1
        # BrowserMatch "Outlook-iOS" MELLON_DISABLE_SAMESITE=1
        # BrowserMatch "UCBrowser\/(8|9|10|11)\.(\d+)\.(\d+)[\.\d]* " MELLON_DISABLE_SAMESITE=1
        # BrowserMatch "UCBrowser\/12\.13\.[0-1][\.\d]* " MELLON_DISABLE_SAMESITE=1
        # BrowserMatch "UCBrowser\/12\.1[0-2]\.(\d+)[\.\d]* " MELLON_DISABLE_SAMESITE=1
        # BrowserMatch "UCBrowser\/12\.\d\.(\d+)[\.\d]* " MELLON_DISABLE_SAMESITE=1
        # BrowserMatch "Chrom[^ \/]+\/6[0-6][\.\d]* " MELLON_DISABLE_SAMESITE=1
        # BrowserMatch "Chrom[^ \/]+\/5[1-9][\.\d]* " MELLON_DISABLE_SAMESITE=1


        # MellonUser selects which attribute we should use for the username.
        # The username is passed on to other apache modules and to the web
        # page the user visits. NAME_ID is an attribute which we set to
        # the id we get from the IdP.
        # Note: If MellonUser refers to a multi-valued attribute, any single
        # value from that attribute may be used. Do not rely on it selecting a
        # specific value.
        # Default: MellonUser "NAME_ID"
        MellonUser "NAME_ID"

        # MellonIdP selects in which attribute we should dump the remote 
        # IdP entityId. This is passed to other apache modules and to 
        # the web pages the user visits.
        # Default: none
        # MellonIdP "IDP"

        # MellonSetEnv configuration directives allows you to map
        # attribute names received from the IdP to names you choose
        # yourself. The syntax is 'MellonSetEnv <local name> <IdP name>'.
        # You can list multiple MellonSetEnv directives.
        # Default. None set.
        MellonSetEnv "e-mail" "mail"

        # MellonSetEnvNoPrefix is identical to MellonSetEnv, except this
        # does not prepend 'MELLON_' to the constructed environment variable.
        # The syntax is 'MellonSetEnvNoPrefix <local name> <IdP name>'.
        # You can list multiple MellonSetEnvNoPrefix directives.
        # Default. None set.
        MellonSetEnvNoPrefix "DISPLAY_NAME" "displayName"

        # MellonEnvPrefix changes the string the variables passed from the
        # IdP are prefixed with.
        # Default: MELLON_
        MellonEnvPrefix "NOLLEM_"

        # MellonMergeEnvVars merges multiple values of environment variables
        # set using MellonSetEnv into single variable:
        # ie: MYENV_VAR => val1;val2;val3 instead of default behaviour of:
        #     MYENV_VAR_0 => val1, MYENV_VAR_1 => val2 ... etc.
        # Second optional parameter specifies the separator, to override the
        # default semicolon.
        # Default: MellonMergeEnvVars Off
        MellonMergeEnvVars On
        MellonMergeEnvVars On ":"

        # MellonEnvVarsIndexStart specifies if environment variables for
        # multi-valued attributes should start indexing from 0 or 1
        # The syntax is 'MellonEnvVarsIndexStart <0|1>'.
        # Default: MellonEnvVarsIndexStart 0
        MellonEnvVarsIndexStart 1

        # MellonEnvVarsSetCount specifies if number of values for any given
        # attribute should also be stored in variable suffixed _N.
        # ie: if user is member of two groups, the following will be set:
        #     MELLON__groups=group1
        #     MELLON__groups_0=group1
        #     MELLON__groups_1=group2
        #     MELLON__groups_N=2
        # Default: MellonEnvVarsSetCount Off
        MellonEnvVarsSetCount On

        # MellonRequire allows you to limit access to those with specific
        # attributes. The syntax is
        # 'MellonRequire <attribute name> <list of valid values>'.
        # Note that the attribute name is the name we received from the
        # IdP.
        #
        # If you don't list any MellonRequire directives (and any 
        # MellonCond directives, see below), then any user authenticated 
        # by the IdP will have access to this service. If you list several 
        # MellonRequire directives, then all of them will have to match.
        # If you use multiple MellonRequire directive on the same 
        # attribute, the last overrides the previous ones.
        #
        # Default: None set.
        MellonRequire "eduPersonAffiliation" "student" "employee"

        # MellonCond provides the same function as MellonRequire, with
        # extra functionality (MellonRequire is retained for backward
        # compatibility). The syntax is
        # 'MellonCond <attribute name> <value> [<options>]'
        #
        # <value> is an attribute value to match. Unlike with MellonRequire, 
        # multiples values are not allowed.
        #
        # If the [REG] flag is specified (see below), <value> is a regular 
        # expression. The syntax for backslash escape is the same as in
        # Apache's <LocationMatch>'s directives. 
        #
        # Format strings are substituted into <value> prior evaluation. 
        # Here are the supported syntaxes:
        #    %n       With n being a digit between 0 and 9. If [REG,REF] 
        #             flags (see below) were used in an earlier matching 
        #             MellonCond, then regular expression back references
        #             are substituted.
        #    %{num}   Same as %n, with num being a number that may be 
        #             greater than 9.
        #    %{ENV:x} Substitute Apache environment variable x.
        #    %%       Escape substitution to get a literal %.
        #
        # <options> is an optional, comma-separated list of option
        # enclosed with brackets. Here is an example: [NOT,NC]
        # The valid options are:
        #    OR  If this MellonCond evaluated to false, then the
        #        next one will be checked. If it evaluates to true,
        #        then the overall check succeeds.
        #    NOT This MellonCond evaluates to true if the attribute 
        #        does not match the value.
        #    SUB Substring match, evaluates to true if value is
        #        included in attribute.
        #    REG Value to check is a regular expression.
        #    NC  Perform case insensitive match. 
        #    MAP Attempt to search an attribute with name remapped by
        #        MellonSetEnv. Fallback to non remapped name if not
        #        found.
        #    REF Used with REG, track regular expression back references,
        #        So that they can be substituted in an upcoming 
        #        MellonCond directive.
        #        
        # It is allowed to have multiple MellonCond on the same 
        # attribute, and to mix MellonCond and MellonRequire. 
        # Note that this can create tricky situations, since the OR
        # option has effect on a following MellonRequire directive.
        # 
        # Default: none set
        # MellonCond "mail" "@example\.net$" [OR,REG]
        # MellonCond "mail" "@example\.com$" [OR,REG]
        # MellonCond "uid" "superuser"

        # MellonEndpointPath selects which directory mod_auth_mellon
        # should assume contains the SAML 2.0 endpoints. Any request to
        # this directory will be handled by mod_auth_mellon.
        #
        # The path is the full path (from the root of the web server) to
        # the directory. The directory must be a sub-directory of this
        # <Location ...>.
        # Default: MellonEndpointPath "/mellon"
        MellonEndpointPath "/secret/endpoint"

        # MellonDefaultLoginPath is the location where one should be
        # redirected after an IdP-initiated login. Default is "/"
        # Default: MellonDefaultLoginPath "/"
        MellonDefaultLoginPath "/"

        # MellonSessionLength sets the maximum lifetime of a session, in
        # seconds. The actual lifetime may be shorter, depending on the
        # conditions received from the IdP. The default length is 86400
        # seconds, which is one day.
        # Default: MellonSessionLength 86400
        MellonSessionLength 86400

        # MellonSessionIdleTimeout represents the amount of time a user
        # can be inactive before the user's session times out in seconds.
        # This parameter is disabled by default to maintain backward 
        # compatibility.
        # Default: MellonSessionIdleTimeout -1 (disabled)
        MellonSessionIdleTimeout -1

        # MellonNoCookieErrorPage is the full path to a page which
        # mod_auth_mellon will redirect the user to if he returns from the
        # IdP without a cookie with a session id.
        # Note that the user may also get this error if he for some reason
        # loses the cookie between being redirected to the IdP's login page
        # and returning from it.
        # If this option is unset, then mod_auth_mellon will return a
        # 400 Bad Request error if the cookie is missing.
        # Default: unset
        MellonNoCookieErrorPage "https://example.com/no_cookie.html"

        # MellonSPMetadataFile is the full path to the file containing
        # the metadata for this service provider.
        # This option is optional. If set, this metadata will be used
        # to configure Mellon. If the XML contains multiple entities, the
        # the first one will be used. This XML will also be published at
        # Mellon's metadata endpoint.
        # Default: None set.
        MellonSPMetadataFile /etc/apache2/mellon/sp-metadata.xml

        # This option can be used to explicitly set the entityID Mellon
        # will present itself as, in authentication and autogenerated metadata.
        # If no entityID is supplied via MellonSPMetadataFile or MellonSPentityID,
        # it will be set to Mellon's metadata URL (https://<yourhost>/mellon/metadata).
        # MellonSPentityId "https://www.example.net/foo"
        #
        # If you choose to autogenerate metadata, these options 
        # can be used to fill the <Organization> element. They
        # all follow the syntax "option [lang] value":
        # MellonOrganizationName "random-service"
        # MellonOrganizationDisplayName "en" "Random service"
        # MellonOrganizationDisplayName "fr" "Service quelconque"
        # MellonOrganizationURL "http://www.espci.fr"

        # MellonSPPrivateKeyFile is a .pem file which contains the private
        # key of the service provider. The .pem-file cannot be encrypted
        # with a password. The private key only needs to be readable by root.
        # Default: None set.
        MellonSPPrivateKeyFile /etc/apache2/mellon/sp-private-key.pem

        # MellonSPCertFile is a .pem file with the certificate for the
        # service provider. This directive is optional.
        # Default: None set.
        MellonSPCertFile /etc/apache2/mellon/sp-cert.pem

        # MellonIdPMetadataFile is the full path to the file which contains
        # metadata for the IdP you are authenticating against. This
        # directive is required.
        #
        # An optional validating chain can be supplied as a second argument
        # to MellonIdPMetadataFile. If omitted, no metadata validation will
        # take place.
        #
        # Multiple IdP metadata can be configured by pointing to an
        # XML file containing descriptors for multiple IdPs, or you
        # can repeat this directive for each different IdP.
        # If you have multiple IdPs enabled, you will need to provide a
        # discovery method to pick the right IdP for this user (see below),
        # if none is given, Mellon will fall back to the first listed.
        #
        # Default: None set.
        MellonIdPMetadataFile /etc/apache2/mellon/idp-metadata.xml

        # MellonIdPMetadataGlob is a glob(3) pattern enabled alternative
        # to MellonIdPMetadataFile.
        #
        # Default: None set.
        #MellonIdPMetadataGlob /etc/apache2/mellon/*-metadata.xml

        # MellonIdPCAFile is the full path to the certificate of the
        # certificate authority. This can be used instead of an
        # certificate for the IdP.
        # Default: None set.
        MellonIdPCAFile /etc/apache2/mellon/ca.pem

        # MellonIdPIgnore lists IdP entityId that should not loaded
        # from XML federation metadata files. This is useful if an
        # IdP cause bugs. Multiple entityId may be specified through
        # single MellonIdPIgnore, and multiple MellonIdPIgnore are allowed.
        # Default: None set.
        #MellonIdPIgnore "https://bug.example.net/saml/idp"

        # MellonDiscoveryURL is the URL for IdP discovery service. 
        # This is used for selecting among multiple configured IdP.
        # On initial user authentication, it is redirected to the
        # IdP discovery URL, with the following arguments set:
        #
        #   entityID         SP entityId URL, where our metadata 
        #                    are published.
        #   returnIDParam    Argument that IdP discovery must send back.
        #   return           Return URL the IdP discovery should return to.
        #
        # The IdP discovery must redirect the user to the return URL, 
        # with returnIDParam set to the selected IdP entityId.
        # 
        # See also MellonProbeDiscovery options below for an alternative
        # way to deal with discovery.
        #
        # Default: None set.
        MellonDiscoveryURL "http://www.example.net/idp-discovery"

        # MellonProbeDiscoveryTimeout sets the timeout of the
        # IdP probe discovery service, which is available on the
        # probeDisco endpoint.
        #
        # This will cause the SP to send HTTP GET requests on the 
        # configured IdP PorviderID URL. This URL should be used to
        # publish metadata, though this is not mandatory. If the IdP
        # returns an HTTP status 200, then the IdP is selected.
        # If the ProviderID URL requires SSL, MellonIdPCAFile is used
        # as a trusted CA bundle.
        #
        # Default: unset, which means the feature is disabled
        # MellonProbeDiscoveryTimeout 1

        # MellonProbeDiscoveryIdP can be used to restrict the 
        # list of IdPs queried by the IdP probe discovery service.
        # If probe discovery fails and this list is provided, an
        # HTTP error 500 is returned, instead of proceeding
        # with first available IdP.
        #
        # Default unset, which means that all configured IdPs are
        # queried.
        # MellonProbeDiscoveryIdP http://idp1.example.com/saml/metadata
        # MellonProbeDiscoveryIdP http://idp2.example.net/saml/metadata

        # This option will make the SAML authentication assertion 
        # available in the MELLON_SAML_RESPONSE environment
        # variable. This assertion holds a verifiable signature
        # that can be checked again. Default is Off.
        MellonSamlResponseDump Off

        # This option will make the Lasso session available in 
        # the MELLON_SESSION environment variable. Default is Off.
        MellonSessionDump Off

        # This option will request specific authentication security-level
        # through the AuthnContextClassRef element of the AuthnRequest It will
        # also request enforcement of this level when receiving an
        # authenticating Assertion.
        # It is possible to give multiple values.
        # If the assertion does not have the required security level, an HTTP
        # Forbidden status code is returned to the browser.
        # MellonAuthnContextClassRef "urn:oasis:names:tc:SAML:2.0:ac:classes:Kerberos"
        # MellonAuthnContextClassRef "urn:oasis:names:tc:SAML:2.0:ac:classes:PasswordProtectedTransport"
        # MellonAuthnContextClassRef "urn:oasis:names:tc:SAML:2.0:ac:classes:SoftwarePKI"

        # This option will set the "Comparsion" attribute within the AuthnRequest
        # It could be set to "exact", "minimum", "maximum" or "better"
        # MellonAuthnContextComparisonType "minimum"

        # MellonSubjectConfirmationDataAddressCheck is used to control
        # the checking of client IP address against the address returned by the
        # IdP in Address attribute of the SubjectConfirmationData node. Can be useful if your SP is
        # behind a reverse proxy or any kind of strange network topology making IP address of client
        # different for the IdP and the SP. Default is on.
        # MellonSubjectConfirmationDataAddressCheck On

        # Does not check signature on logout messages exchanges with idp1
        # MellonDoNotVerifyLogoutSignature http://idp1.example.com/saml/metadata

        # Whether to enable replay of POST requests after authentication. When this option is
        # enabled, POST requests that trigger authentication will be saved until the
        # authentication is completed, and then replayed. If this option isn't enabled,
        # the requests will be turned into normal GET requests after authentication.
        #
        # Note that if this option is enabled, you must also
        # set the MellonPostDirectory option in the server configuration.
        #
        # The default is that it is "Off".
        # MellonPostReplay Off

        # Page to redirect to if the IdP sends an error in response to
        # the authentication request.
        #
        # Example:
        #  MellonNoSuccessErrorPage https://sp.example.org/login_failed.html
        #
        # The default is to not redirect, but rather send a
        # 401 Unautorized error.

        # This option controls whether to include a list of IDP's when
        # sending an ECP PAOS <AuthnRequest> message to an ECP client.
        MellonECPSendIDPList Off

        # This option controls whether the Cache-control header is sent
        # back in responses.
        # Default: On
        # MellonSendCacheControlHeader Off

        # List of domains that we allow redirects to.
        # The special name "[self]" means the domain of the current request.
        # The domain names can also use wildcards.
        #
        # Example:
        #  * Allow redirects to example.com and all subdomains:
        #    MellonRedirectDomains example.com *.example.com
        #  * Allow redirects to the host running mod_auth_mellon, as well as the
        #    web page at www.example.com:
        #    MellonRedirectDomains [self] www.example.com
        #  * Allow redirects to all domains:
        #    MellonRedirectDomains *
        #
        # Default:
        #  MellonRedirectDomains [self]
        MellonRedirectDomains [self]

        # This option controls the signature method used to sign SAML
        # messages generated by Mellon, it may be one of the following
        # (depending if feature was supported when Mellon was built):
        #
        # rsa-sha1
        # rsa-sha256
        # rsa-sha384
        # rsa-sha512
        #
        # Default: rsa-sha256
        # MellonSignatureMethod

</Location>
```

## Service provider metadata

The contents of the metadata will depend on your hostname and on what path
you selected with the MellonEndpointPath configuration directive. You can 
supply the metadata using the MellonSPMetadataFile directive, or you
can just let it be autogenerated.

The following is an example of metadata for the example configuration:

```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<EntityDescriptor
    entityID="https://example.com"
    xmlns="urn:oasis:names:tc:SAML:2.0:metadata">
  <SPSSODescriptor
      AuthnRequestsSigned="false"
      WantAssertionsSigned="false"
      protocolSupportEnumeration="urn:oasis:names:tc:SAML:2.0:protocol">
    <SingleLogoutService
        Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect"
        Location="https://example.com/secret/endpoint/logout" />
    <SingleLogoutService
        Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST"
        Location="https://example.com/secret/endpoint/logout" />
    <NameIDFormat>
      urn:oasis:names:tc:SAML:2.0:nameid-format:transient
    </NameIDFormat>
    <AssertionConsumerService
        index="0"
        isDefault="true"
        Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST"
        Location="https://example.com/secret/endpoint/postResponse" />
  </SPSSODescriptor>
</EntityDescriptor>
```

You should update `entityID="https://example.com"` and the two Location
attributes.  Note that '/secret/endpoint' in the two Location attributes matches
the path set in MellonEndpointPath.

To use the HTTP-Artifact binding instead of the HTTP-POST binding, change
the AssertionConsumerService-element to something like this:

```xml
    <AssertionConsumerService
        index="0"
        isDefault="true"
        Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Artifact"
        Location="https://example.com/secret/endpoint/artifactResponse" />
```

The metadata are published at the 'endpoint/metadata' URL.


## Using mod_auth_mellon

After you have set up mod_auth_mellon, you should be able to visit (in our
example) https://example.com/secret/, and be redirected to the IdP's login
page. After logging in you should be returned to
https://example.com/secret/, and get the contents of that page.

When authenticating a user, mod_auth_mellon will set some environment
variables to the attributes it received from the IdP. The name of the
variables will be `MELLON_<attribute name>`. If you have specified a
different name with the MellonSetEnv or MellonSetEnvNoPrefix configuration
directive, then that name will be used instead. In the case of MellonSetEnv,
the name will still be prefixed by `MELLON_`.

If an attribute has multiple values, then they will be stored as
`MELLON_<name>_0`, `MELLON_<name>_1`, `MELLON_<name>_2`, ...

Since mod_auth_mellon doesn't know which attributes may have multiple
values, it will store every attribute at least twice: once named
`MELLON_<name>`, and once named `<MELLON_<name>_0`.

In the case of multivalued attributes, `MELLON_<name>` will contain the first
value.

NOTE: 

If MellonMergeEnvVars is set to On, multiple values of attributes 
will be stored in single environment variable, separated by semicolons:
```
MELLON_<name>="value1;value2;value3[;valueX]"
```
and variables `MELLON_<name>_0`, `MELLON_<name>_1`, `MELLON_<name>_2`, ...
will not be created.

The following code is a simple PHP script which prints out all the
variables:

```php
<?php
header('Content-Type: text/plain');

foreach($_SERVER as $key => $value) {
  if(str_starts_with($key, 'MELLON_')) {
    echo($key . '=' . $value . "\r\n");
  }
}
```


## Manual login

It is possible to manually trigger login operations. This can be done by
accessing "<endpoint path>/login". That endpoint accepts three parameters:

- `ReturnTo`: A mandatory parameter which contains the URL we should return
  to after login.
- `IdP`: The entity ID of the IdP we should send a login request to. This
  parameter is optional.
- `IsPassive`: This parameter can be set to "true" to send a passive
  authentication request to the IdP.


## Logging out

mod_auth_mellon supports both IdP-initiated and SP-initiated logout
through the same endpoint. The endpoint is located at
"<endpoint path>/logout". "<endpoint path>/logoutRequest" is an alias for
this endpoint, provided for compatibility with version 0.0.6 and earlier of
mod_auth_mellon.

To initiate a logout from your web site, you should redirect or link to
"<endpoint path>/logout?ReturnTo=<url to redirect to after logout>". Note
that the ReturnTo parameter is mandatory. For example, if the web site is
located at "https://www.example.com/secret", and the mellon endpoints are
located under "https://www.example.com/secret/endpoint", then the web site
could contain a link element like the following:

```html
<a href="/secret/endpoint/logout?ReturnTo=https://www.example.org/logged_out.html">Log out</a>
```

This will return the user to "https://www.example.org/logged_out.html"
after the logout operation has completed.


## Invalidating session
It is possible to invalidate the current mod_auth_mellon session, 
without calling SLO. The mod_auth_mellon cookie session will be 
invalidated and the session will be removed from the mod_auth_mellon cache. 
SLO will not be possible after the mod_auth_mellon session is invalidated.
If this functionality is enabled, invalidate the session by calling 
the endpoint "<endpoint path>/invalidate?ReturnTo=<valid url>".
The "ReturnTo" parameter is required.

Here is a sample configuration to enable this feature:
```ApacheConf
MellonEnabledInvalidateSessionEndpoint On
```
Default value is Off


## Send Expect Header
The Expect Header saves an additional network round-trip and is thus a good idea when
the request isn't extremely large and the probability for rejection is low.
For some Apache server versions, the Expect Header is not properly managed and a curl command will
wait for 1 second before sending the body of the request.
If the Expect Header is not present, there won't be wait time in the HTTP-Artifact binding.

Here is a sample configuration to not send the Expect header:
```ApacheConf
MellonSendExpectHeader Off
```
Default value is On


## Probe IdP discovery 

mod_auth_mellon has an IdP probe discovery service that sends HTTP GET
to IdP and picks the first that answers. This can be used as a poor
man's failover setup that redirects to your organisation internal IdP. 
Here is a sample configuration:
```ApacheConf
MellonEndpointPath "/saml"
(...)
MellonDiscoveryUrl "/saml/probeDisco"
MellonProbeDiscoveryTimeout 1
```
The SP will send an HTTP GET to each configured IdP entityId URL until
it gets an HTTP 200 response within the 1 second timeout. It will then 
proceed with that IdP.

If you are in a federation, then your IdP login page will need to provide 
an IdP selection feature aimed at users from other institutions (after
such a choice, the user would be redirected to the SP's /saml/login 
endpoint, with ReturnTo and IdP set appropriately). In such a setup, 
you will want to configure external IdP in mod_auth_mellon, but not 
use them for IdP probe discovery. The MellonProbeDiscoveryIdP 
directive can be used to limit the usable IdP for probe discovery:

```ApacheConf
MellonEndpointPath "/saml"
(...)
MellonDiscoveryUrl "/saml/probeDisco"
MellonProbeDiscoveryTimeout 1
MellonProbeDiscoveryIdP "https://idp1.example.net/saml/metadata"
MellonProbeDiscoveryIdP "https://idp2.example.net/saml/metadata"
```


## Replaying POST requests

By default, POST requests received when the user isn't logged in are turned
into GET requests after authentication. mod_auth_mellon can instead save
the received POST request and replay/repost it after authentication. To
enable this:

1. Create a data directory where mod_auth_mellon can store the saved data:

        mkdir /var/cache/mod_auth_mellon_postdata

2. Set the appropriate permissions on this directory. It needs to be
   accessible for the web server, but nobody else.

        chown www-data /var/cache/mod_auth_mellon_postdata
        chgrp www-data /var/cache/mod_auth_mellon_postdata
        chmod 0700 /var/cache/mod_auth_mellon_postdata

3. Set the MellonPostDirectory option in your server configuration:

        MellonPostDirectory "/var/cache/mod_auth_mellon_postdata"

4. Enable POST replay functionality for the locations you want:

        <Location /secret>
            MellonEnable auth
            [...]
            MellonPostReplay On
        </Location>

After you restart Apache to activate the new configuration, any POST
requests that trigger authentication should now be stored while the
user logs in.


## Example to support both SAML and different auth methods

The below snippet will allow for preemptive basic auth (such as from a REST client)
for the "/auth" path, but if accessed interactively will trigger SAML auth with
mod_auth_mellon.

```ApacheConf
<Location />
  MellonEnable "info"
  MellonVariable "cookie"
  MellonEndpointPath "/sso"
  Mellon... # Other parameters as needed
</Location>

<Location /auth>
  <If "-n req('Authorization')">
    AuthName "My Auth"
    AuthBasicProvider ldap
    AuthType basic
    AuthLDAP* # Other basic auth config parms as needed

    require ldap-group ....
  </If>
  <Else>
    Require valid-user
    AuthType "Mellon"
    MellonEnable "auth"
  </Else>
</Location>
```

In a similar vain you can exclude a subpath from Mellon authentication by adding
a Location block that exempts the path after:

```ApacheConf
<VirtualHost *:443>
        <Location />
            AuthType "Mellon"
            Require valid-user
            MellonEnable "auth"
            ...

            Require all granted
        </Location>

        <Location /noSSO>
            MellonEnable "off"
            Require all granted
        </Location>
</VirtualHost>
```

## Mellon & User Agent Caching behavior

For each content within an Apache Location enabled with "info" or "auth",
mod_auth_mellon sends by default the HTTP/1.1 Cache-Control header with value
`private, must-revalidate`:

- `private` protects content against caching by any proxy servers.
- `must-revalidate` obligates the user agent to revalidate maybe locally
  cached or stored content each time on accessing location.

This default behavior ensures that the user agent never shows cached static
HTML pages after logout without revalidating. So the user can't be
misled about a malfunction of the logout procedure. Revalidating content after
logout leads to a new authentication procedure via mellon.

But mod_auth_mellon will never prohibit specifically any user agent from
caching or storing content locally, that has to be revalidated. So that during
the session, a user agent only revalidates data by the server `304 Not Modified`
response, and does not have to download content again.

For special content types like images it could make sense to disable
revalidation completely, so that user agents can provide cached and stored
content directly to the user. This can be achieved by using other Apache
modules mod_headers and mod_setenvif. E.g. for PNG images:

* Using Apache 2.2 configuration options:

      SetEnvIf Request_URI "\.png$" DISABLE_REVALIDATION
      Header always unset Cache-Control env=DISABLE_REVALIDATION

* In Apache 2.4, there's a shorter notation:

      Header always unset Cache-Control expr=%{CONTENT_TYPE}==image/png

Editing, appending, and overwriting headers is possible in other cases.


## Reporting security vulnerabilities

For reporting security vulnerabilities in mod_auth_mellon, please contact
the maintainers directly at the following email address:

  jhrozek@redhat.com
  simo@redhat.com

This allows us to coordinate the disclosure of the vulnerability with the
fixes for the vulnerability.


## Contributors

Thanks to [Emmanuel Dreyfus](mailto:manu@netbsd.org) for many new features,
including:
- Metadata autogeneration support.
- Support for multiple IdPs.
- IdP discovery service support.
- SOAP logout support.

[Benjamin Dauvergne](mailto:bdauvergne@entrouvert.com) has contributed many
patches, both with bugfixes and new features:
- Cookie settings, for specifying domain and path of cookie.
- Support for SAML 2.0 authentication contexts.
- Support for running behind a reverse proxy.
- Logout improvements, including support for local logout.

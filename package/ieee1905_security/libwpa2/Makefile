include $(TOPDIR)/rules.mk

PKG_NAME:=libwpa2
PKG_RELEASE:=1

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
#  PKG_REV:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
#  PKG_VERSION:=g$(PKG_REV)
  PKG_VERSION:=3.0.1500.042
endif
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/libwpa2
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros, Inc.
  TITLE:=WPA2 Library
  DEPENDS:=
  SUBMENU:=IEEE1905.1 features
endef

define Package/libwpa2/config
  $(call Package/libwpa2/override_version,$(PKG_NAME))
  $(call Package/libwpa2/override_source_path,$(PKG_NAME))
endef

define Package/libwpa2/description
  This Package contains the wpa2 library
endef

ifneq ($(CONFIG_QCA_IEEE1905SECU_USE_DNI_GIT),)
PKG_GIT_DIR=qsdk-ieee1905-security.git
PKG_GIT_SRC_SUBDIR=src/libwpa2

define Build/Prepare
	$(CP) $(GIT_HOME)/qsdk-ieee1905-security.git/$(PKG_GIT_SRC_SUBDIR)/* $(PKG_BUILD_DIR)/
endef
endif


define Package/libwpa2/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/install/lib/libwpa2.so $(1)/usr/lib/
endef

LIBWPA2_MAKEOPTS:= \
        TOOLPREFIX=$(TARGET_CROSS) \
        EXTRA_CFLAGS="$(TARGET_CFLAGS)" \
        EXTRA_LDFLAGS="$(TARGET_LDFLAGS)" \
        INSTALL_ROOT=$(PKG_BUILD_DIR)/install

define Build/Compile
	mkdir -p $(PKG_BUILD_DIR)/install
	mkdir -p $(PKG_BUILD_DIR)/install/lib/
	mkdir -p $(PKG_BUILD_DIR)/install/include/
	$(CP) $(PKG_BUILD_DIR)/libwpa2.so $(PKG_BUILD_DIR)/install/lib
endef

INSTALL_LIBS:= libwpa2

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include/wpa2
	$(if $(INSTALL_LIBS),$(CP) $(foreach lib,$(INSTALL_LIBS),$(PKG_BUILD_DIR)/install/lib/$(lib).so) $(1)/usr/lib)
	$(if $(INSTALL_HEADERS),$(CP) $(foreach header,$(INSTALL_HEADERS),$(PKG_BUILD_DIR)/install/include/$(header)) $(1)/usr/include/wpa2)
endef

$(eval $(call BuildPackage,libwpa2))


include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-libhyfi-bridge
PKG_RELEASE:=1

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
PKG_REV:=e30b131
PKG_VERSION:=ge30b131
endif
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

QCALIBHYFI_BRIDGE_MAKE_OPTS:= \
        CROSS=$(TARGET_CROSS) \
        GWLIB=$(STAGING_DIR)/usr/lib \
        GWINCLUDE=$(STAGING_DIR)/usr/include/qca \
        HYFIDIR=$(STAGING_DIR)/usr/include/hyfibr \
        HYFIMCDIR=$(STAGING_DIR)/usr/include/hyfibr

include $(INCLUDE_DIR)/package.mk

PLATFORM_PATCH_DIR:=./patches

define Package/qca-libhyfi-bridge/config
  $(call Package/$(PKG_NAME)/override_version,$(PKG_NAME))
  $(call Package/$(PKG_NAME)/override_source_path,$(PKG_NAME))
  source "$(SOURCE)/Config.in"
endef

define Package/qca-libhyfi-bridge
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros, Inc.
  DEPENDS:=+kmod-qca-hyfi-bridge
  TITLE:=Hy-Fi Bridging API Library
  SUBMENU:=Hy-Fi features
endef

define Package/qca-libhyfi-bridge/description
  This package installs the Hy-Fi Bridging API Library
endef

INSTALL_LIBS:= libhyfi-bridge

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include/libhyfibr
	$(if $(INSTALL_LIBS),$(CP) $(foreach lib,$(INSTALL_LIBS),$(PKG_BUILD_DIR)/install/lib/$(lib).so) $(1)/usr/lib)
	$(if $(INSTALL_HEADERS),$(CP) $(foreach header,$(INSTALL_HEADERS),$(PKG_BUILD_DIR)/install/include/$(header)) $(1)/usr/include/libhyfibr)
endef

ifneq ($(CONFIG_QCA_HYFI_USE_DNI_GIT),)
PKG_GIT_DIR=qsdk-hyfi.git
PKG_GIT_SRC_SUBDIR=src/qca-libhyfi-bridge

define Build/Prepare
	$(CP) $(GIT_HOME)/qsdk-hyfi.git/$(PKG_GIT_SRC_SUBDIR)/* $(PKG_BUILD_DIR)/
endef
endif

define Build/Compile
	mkdir -p $(PKG_BUILD_DIR)/install/lib
	mkdir -p $(PKG_BUILD_DIR)/install/include
	$(CP) $(PKG_BUILD_DIR)/libhyfi-bridge.so $(PKG_BUILD_DIR)/install/lib
endef

define Package/qca-libhyfi-bridge/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/install/lib/libhyfi-bridge.so $(1)/usr/lib
endef

$(eval $(call BuildPackage,qca-libhyfi-bridge))

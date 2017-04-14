include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-hyctl
PKG_VERSION:=g13f0a6c

PKG_RELEASE:=1

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
PKG_REV:=13f0a6c
PKG_VERSION:=g13f0a6c
endif
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

QCAHYCTL_MAKE_OPTS:= \
        CROSS=$(TARGET_CROSS) \
        GWLIB=$(STAGING_DIR)/usr/lib \
        GWINCLUDE=$(STAGING_DIR)/usr/include/qca \
        HYFIDIR=$(STAGING_DIR)/usr/include/hyfibr \
        HYFIMCDIR=$(STAGING_DIR)/usr/include/hyfibr \
        HYFIBRLIBDIR=$(STAGING_DIR)/usr/include/libhyfibr \
        INSTALL_ROOT=$(PKG_BUILD_DIR)/install

include $(INCLUDE_DIR)/package.mk

define Package/qca-hyctl/config
  $(call Package/$(PKG_NAME)/override_version,$(PKG_NAME))
  $(call Package/$(PKG_NAME)/override_source_path,$(PKG_NAME))
endef

define Package/qca-hyctl
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros, Inc.
  DEPENDS:=+kmod-qca-hyfi-bridge +qca-libhyfi-bridge
  TITLE:=Hy-Fi Control and Configuration command line utility
  SUBMENU:=Hy-Fi features
endef

define Package/qca-hyctl/description
  This package installs the Hy-Fi control command line utility
endef

ifneq ($(CONFIG_QCA_HYFI_USE_DNI_GIT),)
PKG_GIT_DIR=qsdk-hyfi.git
PKG_GIT_SRC_SUBDIR=src/qca-hyctl

define Build/Prepare
	$(CP) $(GIT_HOME)/qsdk-hyfi.git/$(PKG_GIT_SRC_SUBDIR)/* $(PKG_BUILD_DIR)/
endef
endif

define Build/Compile
	mkdir -p $(PKG_BUILD_DIR)/install/lib
	mkdir -p $(PKG_BUILD_DIR)/install/include
	$(CP) $(PKG_BUILD_DIR)/hyctl $(PKG_BUILD_DIR)/install
endef

define Package/qca-hyctl/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/install/hyctl $(1)/usr/sbin
endef

$(eval $(call BuildPackage,qca-hyctl))

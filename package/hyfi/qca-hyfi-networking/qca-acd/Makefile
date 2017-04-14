include $(TOPDIR)/rules.mk
#include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-acd
PKG_VERSION:=master.131

include $(INCLUDE_DIR)/package.mk

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

define Package/qca-acd
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros, Inc.
  DEPENDS:=+qca-ieee1905-init +kmod-qca-hyfi-bridge +qca-hyctl
  TITLE:=Hy-Fi Auto-configuration
  SUBMENU:=Hy-Fi features
endef

define Package/qca-acd/description
  This package installs the Hy-Fi Auto-configuration scripts
endef

ifneq ($(CONFIG_QCA_HYFI_USE_DNI_GIT),)
PKG_GIT_DIR=qsdk-hyfi.git
PKG_GIT_FILES_SUBDIR=feeds/hyfi/qca-hyfi-networking/qca-acd/files

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/files
	$(CP) $(GIT_HOME)/qsdk-hyfi.git/$(PKG_GIT_FILES_SUBDIR)/* $(PKG_BUILD_DIR)/files/
endef
endif

define Build/Compile
endef

define Package/qca-acd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/acd-run.sh $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/acd.init $(1)/etc/init.d/acd
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/acd.config $(1)/etc/config/acd
endef

$(eval $(call BuildPackage,qca-acd))

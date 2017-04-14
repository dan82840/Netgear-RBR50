include $(TOPDIR)/rules.mk

PKG_NAME:=qca-whc-repacd
PKG_SOURCE_PROTO:=git
#PKG_SOURCE_URL:=ssh://qca-git01.qualcomm.com:29418/whc/qca-whc-repacd
PKG_BRANCH:=master
PKG_RELEASE:=1


include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
#  PKG_REV:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
#  PKG_VERSION:=g$(PKG_REV)
  PKG_VERSION:=3.0.1500.042
endif
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=$(PKG_REV)
PKG_SOURCE_MIRROR:=none

include $(INCLUDE_DIR)/package.mk

PLATFORM_PATCH_DIR:=./patches

define Package/qca-whc-repacd
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros, Inc.
  DEPENDS:=+qca-whc-init +qca-ieee1905-init +qca-wsplcd +openssl-util
  TITLE:=Whole Home Coverage RE Placement and Auto-Configuration Daemon
  SUBMENU:=Whole Home Coverage features
endef

define Package/qca-whc-repacd/description
  This package installs the Whole Home Coverage Range Extender Placement and
  Auto-configuration scripts
endef

ifneq ($(CONFIG_QCA_WHC_USE_DNI_GIT),)
PKG_GIT_DIR=qsdk-whc.git
PKG_GIT_FILES_SUBDIR=feeds/whc/qca-whc-repacd/files
PKG_GIT_SRC_SUBDIR=src/qca-whc-repacd

define Build/Prepare
	mkdir -p  $(PKG_BUILD_DIR)/files
	$(CP) $(GIT_HOME)/qsdk-whc.git/$(PKG_GIT_FILES_SUBDIR)/* $(PKG_BUILD_DIR)/files
	$(CP) $(GIT_HOME)/qsdk-whc.git/$(PKG_GIT_SRC_SUBDIR)/* $(PKG_BUILD_DIR)/
endef
endif

define Build/Compile
endef

define Package/qca-whc-repacd/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/repacd.init $(1)/etc/init.d/repacd
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/repacd.config $(1)/etc/config/repacd
	$(INSTALL_DIR) $(1)/etc/hotplug.d/button
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/repacd.button $(1)/etc/hotplug.d/button/53-wps-repacd
	$(INSTALL_DIR) $(1)/lib/functions/
	$(CP) $(PKG_BUILD_DIR)/files/whc-network.sh $(1)/lib/functions/
	$(CP) $(PKG_BUILD_DIR)/lib/*.sh $(1)/lib/functions/
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/repacd-run.sh $(1)/usr/sbin
endef

$(eval $(call BuildPackage,qca-whc-repacd))

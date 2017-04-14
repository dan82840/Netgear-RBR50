include $(TOPDIR)/rules.mk

PKG_NAME:=qca-ieee1905-init
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_VERSION:=3.0.1500.042
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

define Package/qca-ieee1905-init
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros
  TITLE:=IEEE1905.1 Init Script Helpers
  DEPENDS:=
  SUBMENU:=IEEE1905.1 features
endef

define Package/qca-ieee1905-init/config
  $(call Package/qca-ieee1905-init/override_version,$(PKG_NAME))
  $(call Package/qca-ieee1905-init/override_source_path,$(PKG_NAME))
  source "$(SOURCE)/Config.in"
endef

define Package/qca-ieee1905-init/description
  This Package contains the script helper files for the IEEE1905.1 init scripts
endef

ifneq ($(CONFIG_QCA_IEEE1905SECU_USE_DNI_GIT),)
PKG_GIT_DIR=qsdk-ieee1905-security.git
PKG_GIT_FILES_SUBDIR=feeds/ieee1905-security/qca-ieee1905-security-config/qca-ieee1905-init/files

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/files
	$(CP) $(GIT_HOME)/qsdk-ieee1905-security.git/$(PKG_GIT_FILES_SUBDIR)/* $(PKG_BUILD_DIR)/files/
endef
endif

define Package/qca-ieee1905-init/install
	$(INSTALL_DIR) $(1)/lib/functions/
	$(CP) $(PKG_BUILD_DIR)/files/hyfi-debug.sh $(1)/lib/functions/
	$(CP) $(PKG_BUILD_DIR)/files/hyfi-iface.sh $(1)/lib/functions/
	$(CP) $(PKG_BUILD_DIR)/files/hyfi-network.sh $(1)/lib/functions/
endef

define Build/Compile
	true
endef

$(eval $(call BuildPackage,qca-ieee1905-init))

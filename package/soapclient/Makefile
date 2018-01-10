# 
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id$

include $(TOPDIR)/rules.mk

PKG_NAME:=soapclient
PKG_VERSION:=1.0.0
PKG_RELEASE:=1
PKG_BUILD_DEPENDS:=datalib

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

PKG_GIT_TREEISH:=soapclient-dni09

define Package/soapclient
  SECTION:=net
  CATEGORY:=Network
  TITLE:= soapclient
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/soapclient.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/soapclient/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/soapclient $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/soap_agent $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/satelliteinfo $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/soap_check_fw $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/soap_agent.init $(1)/etc/init.d/soap_agent
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/checkfirmware.sh $(1)/usr/sbin/checkfirmware.sh
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/check_wps_soap $(1)/usr/sbin/check_wps_soap
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/soap_wifi_action.sh $(1)/usr/sbin/soap_wifi_action.sh
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/check_soapagent.sh $(1)/usr/sbin/check_soapagent.sh
	$(INSTALL_BIN) ./files/check_satellite_allconfig $(1)/usr/sbin/check_satellite_allconfig
endef

$(eval $(call BuildPackage,soapclient))

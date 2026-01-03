-- Retainer System Tables
-- Migration to add support for the retainer system

-- Main retainer information table
CREATE TABLE IF NOT EXISTS `chararetainerinfo` (
  `RetainerId` BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `CharacterId` INT(20) NOT NULL,
  `DisplayOrder` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Slot index 0-9',
  `Name` VARCHAR(32) NOT NULL,
  `Personality` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1' COMMENT '1-6: Cheerful/Wild/Coolheaded/Carefree/Problematic/Playful',
  `Level` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1',
  `ClassJob` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT '0=None, matches ClassJob sheet',
  `Exp` INT(10) UNSIGNED NOT NULL DEFAULT '0',
  `CityId` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Assigned city-state',
  `Gil` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Gil held by retainer',
  `ItemCount` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Items in retainer inventory',
  `SellingCount` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Items listed on market',
  `VentureId` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Current venture ID (0=none)',
  `VentureComplete` TINYINT(1) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Venture complete flag',
  `VentureStartTime` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix timestamp of venture start',
  `VentureCompleteTime` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix timestamp of venture completion',
  `Customize` VARBINARY(26) DEFAULT NULL COMMENT 'Character appearance data (same as player)',
  `IsActive` TINYINT(1) UNSIGNED NOT NULL DEFAULT '1',
  `IsRename` TINYINT(1) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Rename token used flag',
  `Status` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  `CreateTime` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix timestamp of creation',
  `UPDATE_DATE` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`RetainerId`),
  INDEX `idx_character` (`CharacterId`),
  UNIQUE INDEX `idx_name` (`Name`) COMMENT 'Retainer names must be unique per world'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Retainer information for player characters';

-- Retainer inventory storage
-- Uses same structure as player inventory for consistency
CREATE TABLE IF NOT EXISTS `chararetaineriteminventory` (
  `RetainerId` BIGINT(20) UNSIGNED NOT NULL,
  `storageId` INT(10) NOT NULL COMMENT '10000-10006 for retainer bags',
  `type` INT(5) DEFAULT '0',
  `idx` INT(5) DEFAULT '0',
  `container_0` INT(20) DEFAULT '0',
  `container_1` INT(20) DEFAULT '0',
  `container_2` INT(20) DEFAULT '0',
  `container_3` INT(20) DEFAULT '0',
  `container_4` INT(20) DEFAULT '0',
  `container_5` INT(20) DEFAULT '0',
  `container_6` INT(20) DEFAULT '0',
  `container_7` INT(20) DEFAULT '0',
  `container_8` INT(20) DEFAULT '0',
  `container_9` INT(20) DEFAULT '0',
  `container_10` INT(20) DEFAULT '0',
  `container_11` INT(20) DEFAULT '0',
  `container_12` INT(20) DEFAULT '0',
  `container_13` INT(20) DEFAULT '0',
  `container_14` INT(20) DEFAULT '0',
  `container_15` INT(20) DEFAULT '0',
  `container_16` INT(20) DEFAULT '0',
  `container_17` INT(20) DEFAULT '0',
  `container_18` INT(20) DEFAULT '0',
  `container_19` INT(20) DEFAULT '0',
  `container_20` INT(20) DEFAULT '0',
  `container_21` INT(20) DEFAULT '0',
  `container_22` INT(20) DEFAULT '0',
  `container_23` INT(20) DEFAULT '0',
  `container_24` INT(20) DEFAULT '0',
  `UPDATE_DATE` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`RetainerId`, `storageId`),
  CONSTRAINT `fk_retaineritem_retainer` FOREIGN KEY (`RetainerId`) 
    REFERENCES `chararetainerinfo` (`RetainerId`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Retainer inventory containers';

-- Retainer item details (same structure as player items)
CREATE TABLE IF NOT EXISTS `chararetaineritem` (
  `RetainerId` BIGINT(20) UNSIGNED NOT NULL,
  `itemId` INT(20) NOT NULL DEFAULT '0',
  `storageId` INT(5) NOT NULL DEFAULT '0',
  `containerIndex` INT(5) NOT NULL DEFAULT '0',
  `stack` INT(10) DEFAULT '1',
  `catalogId` INT(10) DEFAULT '0',
  `reservedFlag` INT(10) DEFAULT '0',
  `signatureId` INT(20) DEFAULT '0',
  `flags` INT(3) DEFAULT '0',
  `durability` INT(5) DEFAULT '30000',
  `refine` INT(5) DEFAULT '0',
  `materia_0` INT(5) DEFAULT '0',
  `materia_1` INT(5) DEFAULT '0',
  `materia_2` INT(5) DEFAULT '0',
  `materia_3` INT(5) DEFAULT '0',
  `materia_4` INT(5) DEFAULT '0',
  `stain` INT(3) DEFAULT '0',
  `pattern` INT(10) DEFAULT '0',
  `buffer_0` INT(3) DEFAULT '0',
  `buffer_1` INT(3) DEFAULT '0',
  `buffer_2` INT(3) DEFAULT '0',
  `buffer_3` INT(3) DEFAULT '0',
  `buffer_4` INT(3) DEFAULT '0',
  `deleted` INT(1) DEFAULT '0',
  `UPDATE_DATE` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`RetainerId`, `itemId`),
  CONSTRAINT `fk_retaineritemdetail_retainer` FOREIGN KEY (`RetainerId`) 
    REFERENCES `chararetainerinfo` (`RetainerId`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Individual items in retainer inventory';

-- Market board listings for retainers
CREATE TABLE IF NOT EXISTS `chararetainermarket` (
  `ListingId` BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `RetainerId` BIGINT(20) UNSIGNED NOT NULL,
  `ItemId` INT(20) NOT NULL COMMENT 'Reference to chararetaineritem.itemId',
  `CatalogId` INT(10) NOT NULL COMMENT 'Item catalog ID for search',
  `Stack` INT(10) NOT NULL DEFAULT '1',
  `Price` INT(10) UNSIGNED NOT NULL COMMENT 'Gil price per unit',
  `TaxRate` TINYINT(3) UNSIGNED NOT NULL DEFAULT '5' COMMENT 'Tax percentage',
  `ListTime` INT(10) UNSIGNED NOT NULL COMMENT 'Unix timestamp of listing',
  `ContainerIndex` TINYINT(3) UNSIGNED NOT NULL COMMENT 'Slot in retainer market storage',
  `SubQuality` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'HQ flag',
  `MateriaCount` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  `UPDATE_DATE` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`ListingId`),
  INDEX `idx_retainer` (`RetainerId`),
  INDEX `idx_catalog` (`CatalogId`),
  INDEX `idx_price` (`Price`),
  CONSTRAINT `fk_market_retainer` FOREIGN KEY (`RetainerId`) 
    REFERENCES `chararetainerinfo` (`RetainerId`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Market board listings';

-- Sales history for retainers
CREATE TABLE IF NOT EXISTS `chararetainersaleshistory` (
  `HistoryId` BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `RetainerId` BIGINT(20) UNSIGNED NOT NULL,
  `CatalogId` INT(10) NOT NULL,
  `Stack` INT(10) NOT NULL,
  `SellPrice` INT(10) UNSIGNED NOT NULL,
  `BuyerName` VARCHAR(32) NOT NULL,
  `SellTime` INT(10) UNSIGNED NOT NULL COMMENT 'Unix timestamp of sale',
  `SubQuality` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  `MateriaCount` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  `UPDATE_DATE` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`HistoryId`),
  INDEX `idx_retainer` (`RetainerId`),
  INDEX `idx_time` (`SellTime`),
  CONSTRAINT `fk_history_retainer` FOREIGN KEY (`RetainerId`) 
    REFERENCES `chararetainerinfo` (`RetainerId`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Retainer sales history (last 20 per retainer)';

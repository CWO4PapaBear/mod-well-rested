-- Apply once to the characters database, not world/auth. Idempotent, no core table changes.
CREATE TABLE IF NOT EXISTS `mod_well_rested_character` (
  `guid` INT UNSIGNED NOT NULL,
  `remaining_ms` INT UNSIGNED NOT NULL DEFAULT 0,
  `fraction` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS `mod_well_rested_schema` (
  `id` TINYINT UNSIGNED NOT NULL,
  `version` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
INSERT IGNORE INTO `mod_well_rested_schema` (`id`,`version`) VALUES (1,1);

-- Adminer 4.8.1 MySQL 5.5.5-10.8.3-MariaDB-1:10.8.3+maria~jammy dump

SET NAMES utf8;
SET time_zone = '+00:00';
SET foreign_key_checks = 0;
SET sql_mode = 'NO_AUTO_VALUE_ON_ZERO';

SET NAMES utf8mb4;

CREATE DATABASE `battery_data` /*!40100 DEFAULT CHARACTER SET utf8mb4 */;
USE `battery_data`;

DROP TABLE IF EXISTS `role`;
CREATE TABLE `role` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(80) DEFAULT NULL,
  `description` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `role` (`id`, `name`, `description`) VALUES
(1,	'user',	NULL),
(2,	'superuser',	NULL);

DROP TABLE IF EXISTS `roles_users`;
CREATE TABLE `roles_users` (
  `user_id` int(11) DEFAULT NULL,
  `role_id` int(11) DEFAULT NULL,
  KEY `user_id` (`user_id`),
  KEY `role_id` (`role_id`),
  CONSTRAINT `roles_users_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `user` (`id`),
  CONSTRAINT `roles_users_ibfk_2` FOREIGN KEY (`role_id`) REFERENCES `role` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `roles_users` (`user_id`, `role_id`) VALUES
(1,	2),
(1,	1),
(2,	1),
(3,	1),
(4,	1),
(5,	1),
(6,	1);

DROP TABLE IF EXISTS `user`;
CREATE TABLE `user` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `first_name` varchar(255) NOT NULL,
  `last_name` varchar(255) DEFAULT NULL,
  `email` varchar(255) NOT NULL,
  `password` varchar(255) NOT NULL,
  `active` tinyint(1) DEFAULT NULL,
  `confirmed_at` datetime DEFAULT NULL,
  `fs_uniquifier` varchar(64) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `email` (`email`),
  UNIQUE KEY `fs_uniquifier` (`fs_uniquifier`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `user` (`id`, `first_name`, `last_name`, `email`, `password`, `active`, `confirmed_at`, `fs_uniquifier`) VALUES
(1,	'user',	NULL,	'user@admin.dev',	'$pbkdf2-sha512$25000$DsH4P.d8D.G8N2asldJaSw$biDYzfN5PA.Ti5mQo0yeZrqscORVD5coA.yTMcTRqpJ3se0mN1AGI/Kdbs3CMKFl2sXNAwDQyE75KedjeGHNmQ',	1,	NULL,	'81ca5fff378a443fb582938b3419589b'),
(2,	'Harry',	'Brown',	'harry.brown@example.com',	'$pbkdf2-sha512$25000$9x7D.L.3NiYEoLS2lpKSUg$xZ/7LbFEQJMxJmZuORuC7Gfj3MyYd83sQqIBiqgwqRx/X3vfrdoswuWpYwLvYZo91T46oIji//FXXm/pag/ZiQ',	1,	NULL,	'98123b5cb5a1473f83e40b7c6b335d6c'),
(3,	'Amelia',	'Smith',	'amelia.smith@example.com',	'$pbkdf2-sha512$25000$RsjZu1cqZexdC4EQgjDG.A$CnQXEAWTrPQ25iLvGJOKVXxrE5ZC8lvtV0cwAi1dtFW3I4PAJKDrs8NCKCxVCPGENv1FNAq5bmqPBa3E87uVFg',	1,	NULL,	'b1741c7d49314ea1b57eb27b5352dd7f'),
(4,	'Oliver',	'Patel',	'oliver.patel@example.com',	'$pbkdf2-sha512$25000$cG5NKYUQglBKaS2l9D6HMA$tRlZKHmH34fjwMxD.CFd82dMfU6xEJ8jlQP3jJiEjpf.3adiU1BY9c3UH/xOeB8R7smGbF6izPRv2mdVGJs8/A',	1,	NULL,	'7ea2e59d2baa473ba2b7a8f333e1782e'),
(5,	'Jack',	'Jones',	'jack.jones@example.com',	'$pbkdf2-sha512$25000$Wus9R0ipNaZUCiFEaO1dCw$qzHLWQx6QWb1tazFGYGFzPTRYpSK5m80k5axNcMDUndXvaGrUtkY3dpJaCQn9pMuO9ERrRehTO18NrvoPPUtzw',	1,	NULL,	'9350c08645f544cbbdb5f9886c28ffda'),
(6,	'Isabella',	'Williams',	'isabella.williams@example.com',	'$pbkdf2-sha512$25000$UKoVIiREaM0ZQ0hpjRHCOA$8/1OUxIXBgbkSiEjnv1DmcQN0O8UWeZ0d7HaGpXS9wOIh/foHJ7ErTg1q8V2pysnwhTP/CIMX3M7obfhbU7zfA',	1,	NULL,	'bd88ff1b7ac84b2ebbd1d66720755dc7');

-- 2025-04-28 13:32:56

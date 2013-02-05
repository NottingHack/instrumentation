-- Table to Nottinghack Google Group emails
drop table if exists gg_addresses;

create table gg_addresses 
(
  ggaddresses_id    int not null auto_increment,
  ggaddress_email   varchar(200),
  ggaddress_name    varchar(200),
  primary key (ggaddresses_id)
) ENGINE = InnoDB; 

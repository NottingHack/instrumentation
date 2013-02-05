-- Table to Nottinghack Google Group emails
drop table if exists gg_summary;

create table gg_summary 
(
  ggemail_id        int not null,
  ggaddresses_id    int not null,
  ggsum_auto_wc     int,
  ggsum_manual_wc   int ,
  ggemail_date      timestamp not null default 0,
  primary key (ggemail_id)
) ENGINE = InnoDB; 

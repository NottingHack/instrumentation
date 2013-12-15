-- Table to Nottinghack Google Group emails
drop table if exists gg_email;

create table gg_email 
(
  ggemail_id        int not null auto_increment,
  ggemail_subj      varchar(200),
  ggemail_body      varchar(10000),
  ggemail_body_wc   varchar(10000),
  ggemail_from      varchar(200),
  ggemail_date      timestamp default CURRENT_TIMESTAMP,
  ggemail_reply_to  varchar(200),
  ggemail_msg_id    varchar(200),
  primary key (ggemail_id)
) ENGINE = InnoDB; 

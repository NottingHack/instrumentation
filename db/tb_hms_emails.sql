drop table if exists hms_emails;

create table hms_emails
(
  hms_email_id  int not null auto_increment,
  member_id     int not null,
  subject       text not null,
  timestamp     timestamp not null default CURRENT_TIMESTAMP,
  primary key (hms_email_id)
) ENGINE = InnoDB; 

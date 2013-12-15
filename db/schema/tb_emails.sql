drop table if exists emails;

create table emails 
(
  email_id        int not null auto_increment,
  member_id       int,
  email_to        varchar(200),
  email_cc        varchar(200),
  email_bcc       varchar(200),
  email_subj      varchar(200),
  email_body      text,
  email_body_alt  text,
  email_status    varchar(16), --  PENDING, SENT, FAILED, BOUNCED [, RECEIVED?]
  email_date      timestamp,
  email_link      int, -- for recieved emails: original email_id 
  primary key (email_id)
) ENGINE = InnoDB; 

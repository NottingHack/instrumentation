
drop table if exists invoices;

create table invoices 
(
  invoice_id        int not null auto_increment,
  member_id         int,
  invoice_from      date,
  invoice_to        date,
  invoice_generated timestamp, 
  invoice_status    varchar(16), -- GENERATING, GENERATED, FAILED
  invoice_amount    int,
  email_id          int,
  primary key (invoice_id)
) ENGINE = InnoDB; 


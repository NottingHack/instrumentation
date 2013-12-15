drop table if exists status;

create table status 
(
  status_id   int not null auto_increment,
  title       varchar(255) not null,
  primary key (status_id)
) ENGINE = InnoDB; 

insert into status (status_id, title) values
(1,'Prospective Member'),
(2,'Waiting for contact details'),
(3,'Waiting for Membership Admin to approve contact details'),
(4,'Waiting for standing order payment'),
(5,'Current Member'),
(6,'Ex Member');

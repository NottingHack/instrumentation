drop table if exists status;

create table status 
(
  status_id   int not null auto_increment,
  title       varchar(255) not null,
  description text NOT NULL,
  primary key (status_id)
) ENGINE = InnoDB; 

insert into status (status_id, title, description) values
(1, 'Prospective Member', 'Interested in the hackspace, we have their e-mail. May be receiving the newsletter'),
(2, 'Current Member', 'Active member'),
(3, 'Ex Member', 'Former member, details only kept for a while'),
(5, 'Pre-Member (stage 1)', 'Member has HMS login details, waiting for them to enter contact details'),
(6, 'Pre-Member (stage 2)', 'Waiting for member-admin to approve contact details'),
(7, 'Pre-Member (stage 3)', 'Waiting for standing order');

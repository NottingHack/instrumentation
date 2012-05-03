drop table if exists products;

create table products 
(
  product_id    int not null auto_increment,
  price         int not null,
  barcode       varchar(25),
  available     int, -- 0=no, 1=yes
  shortdesc     varchar(25),
  longdesc      varchar(512),
  primary key (product_id),
  constraint product_barcode unique (barcode)
) ENGINE = InnoDB; 

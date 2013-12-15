drop table if exists vmc_state;

create table vmc_state 
(
  vmc_ref_id    int not null,
  product_id    int not null,
  primary key (vmc_ref_id)
) ENGINE = InnoDB; 

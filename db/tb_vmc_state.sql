drop table if exists vmc_state;

create table vmc_state 
(
  vmc_ref_id    int not null,
  product_id    int not null,
  constraint vmc_state_map unique (vmc_ref_id, product_id)
) ENGINE = InnoDB; 

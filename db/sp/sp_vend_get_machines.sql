drop procedure if exists sp_vend_get_machines;

/*
  Get details of all known vending machines
*/

DELIMITER //
CREATE PROCEDURE sp_vend_get_machines
(
   IN  p_vmc_id int
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare tran_id   int;
  main: begin  

  select
    vd.vmc_id,
    vd.vmc_description,
    vd.vmc_type,
    vd.vmc_connection,
    vd.vmc_address
  from vmc_details vd
  where vd.vmc_id = p_vmc_id or p_vmc_id = -1;

  end main;
 

END //
DELIMITER ;

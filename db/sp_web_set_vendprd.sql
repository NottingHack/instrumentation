drop procedure if exists sp_web_set_vendprd;

/*


*/

DELIMITER //
CREATE PROCEDURE sp_web_set_vendprd
(
   IN   member_id   int,
   IN   loc_name    varchar(10),
   IN   product_id  int,
   OUT  err         varchar(255)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists      int;
  declare vr_vmc_ref_id  int;
  
  set ck_exists = 0;
  set err = '';
 
  main: begin  

    -- TODO: member_id/permissions check

    select count(*) into ck_exists
    from vmc_ref vr
    where vr.loc_name = loc_name;

    if (ck_exists != 1) then
      set err = 'Failed to find location';
      leave main;
    end if;
    
    select vr.vmc_ref_id
    into vr_vmc_ref_id
    from vmc_ref vr
    where vr.loc_name = loc_name;
    
    if (product_id = -1) then
      delete from vmc_state
      where vmc_ref_id = vr_vmc_ref_id;

      leave main;
    end if;    

    select count(*) into ck_exists
    from products p
    where p.product_id = product_id;

    if (ck_exists != 1) then
      set err = 'Failed to find product';
      leave main;
    end if;    

    select vr.vmc_ref_id
    into vr_vmc_ref_id
    from vmc_ref vr
    where vr.loc_name = loc_name;

    replace into vmc_state (vmc_ref_id, product_id) values (vr_vmc_ref_id, product_id);

  end main;
  

END //
DELIMITER ;


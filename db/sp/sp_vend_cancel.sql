drop procedure if exists sp_vend_cancel;

/*
  Cancel button pushed - so set the vend to canceled. Note that if a VNOK (vend ok)
  message as been set (req_datetime is set in vend_log), then it's too late to cancel,
  as the vending machine should be in the process of vending.
*/

DELIMITER //
CREATE PROCEDURE sp_vend_cancel
(
   IN  rfid_serial  varchar( 50),
   IN  vend_tran_id varchar(  6),
   OUT err          varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare tran_id   int;
  set err = '';

  main: begin  

    -- Check the transaction id & serial id match up with an active vend entry
    select count(*) into ck_exists
    from vend_log v  
    where v.vend_tran_id = vend_tran_id
      and v.rfid_serial = rfid_serial 
      and v.cancelled_datetime is null
      and v.req_datetime is null;
      
    if (ck_exists = 0) then
      set err = 'unable to find matching entry in vend_log (BUG)'; 
      leave main;
    end if;

    update vend_log
    set cancelled_datetime = sysdate()
    where vend_log.vend_tran_id = vend_tran_id;    

  end main;
 

END //
DELIMITER ;

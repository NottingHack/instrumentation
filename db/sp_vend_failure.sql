drop procedure if exists sp_vend_failure;

/*
  Vend machine has told us the vend failed. Update vend_log to show it as failed,
  and set the transaction entry to aborted.
*/

DELIMITER //
CREATE PROCEDURE sp_vend_failure
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
      and v.transaction_id is not null
      and v.failed_datetime is null;
      
    if (ck_exists = 0) then
      set err = 'unable to find matching entry in vend_log (BUG)'; 
      leave main;
    end if;

    select 
      v.transaction_id
    into
      tran_id
    from vend_log v 
    where v.vend_tran_id = vend_tran_id;
    
    call sp_transaction_update(tran_id, 'ABORTED', 'Vend failed', err);
    if (length(err) > 0) then
      leave main;
    end if;
    
    update vend_log
    set failed_datetime = sysdate()
    where vend_log.vend_tran_id = vend_tran_id;    

  end main;
 

END //
DELIMITER ;


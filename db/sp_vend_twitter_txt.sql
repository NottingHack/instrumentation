drop procedure if exists sp_vend_twitter_txt;

/*
  Take a transaction id that relates to a sucsessfull vend, and return a tweet (or blank
  string if not enough data)
*/

DELIMITER //
CREATE PROCEDURE sp_vend_twitter_txt
(
   IN  vend_tran_id varchar(  6),
   OUT tweet        varchar(140)
)
SQL SECURITY DEFINER
BEGIN
  /*declare ck_exists int;
  declare tran_id   int;
 */

  declare continue HANDLER for SQLWARNING begin end;

  main: begin  

    set tweet = "";
    select concat(m.username, ' purchased a ', p.shortdesc, ' from the vending machine')
    into tweet
    from vend_log vl
    inner join vmc_ref vr on vr.loc_encoded = vl.position
    inner join vmc_state vs on vs.vmc_ref_id = vr.vmc_ref_id
    inner join products p on p.product_id = vs.product_id
    inner join members m on m.member_id = vl.member_id
    inner join vmc_details vd on vd.vmc_id = vl.vmc_id
    where vl.vend_tran_id = vend_tran_id
      and vd.vmc_type = 'VEND'; -- Don't want tweets for payments

  end main;

END //
DELIMITER ;

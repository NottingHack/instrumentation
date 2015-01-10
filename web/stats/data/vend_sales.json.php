<?php

include "dbaccess_header.php";

$sql = <<<SQL
  select 
    p.shortdesc as Item, 
    concat('£', cast((p.price/100) as decimal(20,2))) as Price,
    (select count(*) from transactions t where t.product_id = p.product_id) as Purchases,
    case when exists (select null from vmc_state vs where vs.product_id = p.product_id) then 'Yes' else 'No' end as Available
  from products p 
  order by shortdesc;
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>
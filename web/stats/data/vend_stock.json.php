<?php

include "dbaccess_header.php";

$sql = <<<SQL
  select
    vr.loc_name as Position,
    p.shortdesc as Product,
    p.longdesc as Description,
    concat('£', cast((price/100) as decimal(20,2))) as Cost
  from vmc_ref vr
  left outer join vmc_state vs on vr.vmc_ref_id = vs.vmc_ref_id
  left outer join products p on vs.product_id = p.product_id
  order by vr.loc_name;
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>
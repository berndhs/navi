CREATE TABLE "wayparcels" (
  "wayid" INTEGER NOT NULL,
  "parcelid" INTEGER NOT NULL,
   UNIQUE ("wayid") ON CONFLICT IGNORE
);

CREATE TABLE "wayparcels" (
  "wayid" TEXT NOT NULL,
  "parcelid" INTEGER NOT NULL,
   UNIQUE ("wayid") ON CONFLICT IGNORE
);

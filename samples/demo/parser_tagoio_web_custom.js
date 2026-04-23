/**
 * TagoIO Payload Parser - Custom fields
 * Copy/paste this content into the Payload Parser in TagoIO.
 */

const raw = payload.find((x) => ["payload_raw", "payload", "data"].includes(x.variable));

if (raw) {
  try {
    const buffer = Buffer.from(raw.value, "hex");
    const hasHumNum = buffer.length >= 23;
    const receive_epoch = Date.now();
    const timestamp = buffer.readUInt32LE(4);
    const device_date = new Date(timestamp * 1000);
    const receive_date = raw.time ? new Date(raw.time) : new Date(receive_epoch);
    const latency_s = Math.floor(receive_epoch / 1000) - timestamp;

    payload = payload.concat(
      [
        { variable: "seq", value: buffer.readInt32LE(0) },
        { variable: "lat", value: buffer.readInt32LE(8) / 1e7 },
        { variable: "lon", value: buffer.readInt32LE(12) / 1e7 },
        { variable: "alt_m", value: buffer.readInt16LE(16), unit: "m" },
        {
          variable: "alt_ft",
          value: Math.round(buffer.readInt16LE(16) * 3.28084),
          unit: "ft",
        },
        {
          variable: "location",
          value: "Location",
          location: {
            lat: buffer.readInt32LE(8) / 1e7,
            lng: buffer.readInt32LE(12) / 1e7,
          },
        },
        { variable: "temperature_c", value: buffer.readInt8(18), unit: "°C" },
        {
          variable: "temperature_f",
          value: Math.round(buffer.readInt8(18) * 1.8 + 32),
          unit: "°F",
        },
        {
          variable: "batt_volt",
          value: Number((buffer.readUInt16LE(19) / 1000).toFixed(1)),
          unit: "V",
        },
        ...(hasHumNum
          ? [
              { variable: "hum", value: buffer.readUInt8(21), unit: "%" },
              { variable: "num", value: buffer.readUInt8(22) },
            ]
          : []),
        { variable: "device_time", value: device_date.toISOString() },
        { variable: "receive_time", value: receive_date.toLocaleString("en-AU") },
        { variable: "latency_s", value: latency_s },
        { variable: "latency_m", value: Number((latency_s / 60).toFixed(1)) },
      ].map((x) => ({ ...x, serie: raw.serie, time: receive_date }))
    );
  } catch (e) {
    console.error(e);
    payload = [{ variable: "parse_error", value: e.message }];
  }
}

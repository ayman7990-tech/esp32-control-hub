const CACHE='esp32-v2';
self.addEventListener('install',e=>{
  e.waitUntil(caches.open(CACHE).then(c=>c.addAll(['/'])).then(()=>self.skipWaiting()));
});
self.addEventListener('activate',e=>{
  e.waitUntil(caches.keys().then(k=>Promise.all(k.filter(x=>x!==CACHE).map(x=>caches.delete(x)))).then(()=>self.clients.claim()));
});
self.addEventListener('fetch',e=>{
  e.respondWith(fetch(e.request).then(r=>{
    if(r.status===200 && e.request.method==='GET'){
      var clone=r.clone();
      caches.open(CACHE).then(c=>c.put(e.request,clone));
    }
    return r;
  }).catch(()=>caches.match(e.request)));
});

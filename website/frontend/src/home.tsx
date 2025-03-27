import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import './index.css';
import { Building2, Globe, Battery, Users, Lightbulb, Cpu, School, Factory, Settings, Target, Leaf, Coins, Users2, GanttChartSquare, Zap, Award, LineChart, Building, Landmark, FlaskConical } from 'lucide-react';

function App() {
  return (
    <div className="min-h-screen bg-white flex flex-col">
      {/* Hero Section */}
      <div 
        className="relative h-[600px] bg-cover bg-center bg-fixed"
        style={{
          backgroundImage: 'url("https://images.unsplash.com/photo-1497440001374-f26997328c1b?auto=format&fit=crop&q=80")',
        }}
      >
        <div className="absolute inset-0 bg-gradient-to-b from-black/70 to-black/40" />
        <div className="relative container mx-auto px-4 h-full flex items-center">
          <div className="max-w-4xl text-white">
            <h1 className="text-5xl md:text-6xl font-bold mb-6 leading-tight">
              Innovate project (HIGHESS) wins £1.4M for 2nd life mini-grid in Sub-Sahara Africa
            </h1>
            <p className="text-xl md:text-2xl opacity-90 font-light">
              A transformative project revolutionizing energy access in Sub-Saharan Africa
            </p>
          </div>
        </div>
      </div>

      {/* Key Features */}
      <div className="bg-gradient-to-b from-gray-50 to-white py-12">
        <div className="container mx-auto px-4">
          <div className="grid grid-cols-1 md:grid-cols-4 gap-12">
            <div className="flex flex-col items-center text-center group">
              <div className="bg-green-100 p-6 rounded-2xl mb-6 transform transition-transform group-hover:scale-110">
                <Battery className="h-10 w-10 text-green-600" />
              </div>
              <h3 className="text-xl font-semibold mb-3">Sustainable Energy</h3>
              <p className="text-gray-600">High-capacity flexible energy storage systems</p>
            </div>
            <div className="flex flex-col items-center text-center group">
              <div className="bg-blue-100 p-6 rounded-2xl mb-6 transform transition-transform group-hover:scale-110">
                <Globe className="h-10 w-10 text-blue-600" />
              </div>
              <h3 className="text-xl font-semibold mb-3">Global Impact</h3>
              <p className="text-gray-600">Bridging energy access gaps in Sub-Saharan Africa</p>
            </div>
            <div className="flex flex-col items-center text-center group">
              <div className="bg-purple-100 p-6 rounded-2xl mb-6 transform transition-transform group-hover:scale-110">
                <Users className="h-10 w-10 text-purple-600" />
              </div>
              <h3 className="text-xl font-semibold mb-3">Strategic Partnerships</h3>
              <p className="text-gray-600">Collaboration with industry leaders</p>
            </div>
            <div className="flex flex-col items-center text-center group">
              <div className="bg-orange-100 p-6 rounded-2xl mb-6 transform transition-transform group-hover:scale-110">
                <Building2 className="h-10 w-10 text-orange-600" />
              </div>
              <h3 className="text-xl font-semibold mb-3">Innovation Hub</h3>
              <p className="text-gray-600">Advanced research and development facility</p>
            </div>
          </div>
        </div>
      </div>

      {/* About Section */}
      <div className="bg-white py-8">
        <div className="container mx-auto px-4">
          <div className="max-w-6xl mx-auto">
            <h2 className="text-4xl font-bold mb-6 text-center text-gray-900">About HIGHESS</h2>
            <div className="relative">
              <div className="absolute -inset-4 bg-gradient-to-r from-green-100 via-blue-50 to-purple-100 rounded-2xl opacity-10"></div>
              <div className="relative space-y-6">
                <p className="text-2xl font-light leading-relaxed text-gray-800">
                  AceOn Group, UK renewable energy and battery specialists, are very pleased to announce that its development of 'HIGHESS' has been awarded significant grant funding from Innovate UK.
                </p>
                <p className="text-xl text-gray-700 leading-relaxed">
                  This transformative project is set to revolutionise energy access in Sub-Saharan Africa through innovative battery technology and sustainable energy solutions. Commencing in May 2024, this two-year initiative represents a significant step forward in addressing global energy challenges through high-capacity flexible energy storage systems.
                </p>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Main Content */}
      <div className="container mx-auto px-4 py-16">
        <div className="max-w-5xl mx-auto">
          <div className="prose prose-lg max-w-none">
            <h2 className="text-4xl font-bold mb-12 text-gray-900">Our Strategic Partners</h2>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-10 mb-16">
              <div className="bg-white rounded-2xl shadow-xl p-8 border border-gray-100 hover:shadow-2xl transition-shadow duration-300">
                <div className="flex items-center mb-6">
                  <div className="bg-blue-100 p-4 rounded-xl">
                    <Factory className="h-8 w-8 text-blue-600" />
                  </div>
                  <h3 className="text-2xl font-bold ml-6">In2tec</h3>
                </div>
                <p className="text-gray-700 mb-6 text-lg">Leaders in sustainable electronics, bringing expertise in recyclable electronics to advance AceOn's Battery Management System (BMS), Inverter, and Power Conditioning Unit (PCU) developments.</p>
                <ul className="list-disc pl-6 text-gray-700 space-y-2">
                  <li>Advanced BMS Development</li>
                  <li>Sustainable Electronics Design</li>
                  <li>Power Systems Integration</li>
                </ul>
              </div>

              <div className="bg-white rounded-2xl shadow-xl p-8 border border-gray-100 hover:shadow-2xl transition-shadow duration-300">
                <div className="flex items-center mb-6">
                  <div className="bg-purple-100 p-4 rounded-xl">
                    <School className="h-8 w-8 text-purple-600" />
                  </div>
                  <h3 className="text-2xl font-bold ml-6">University of Liverpool</h3>
                </div>
                <p className="text-gray-700 mb-6 text-lg">Academic excellence in energy systems research, contributing expertise in digital twin technology and data analytics for predictive maintenance.</p>
                <ul className="list-disc pl-6 text-gray-700 space-y-2">
                  <li>Energy Systems Research</li>
                  <li>Digital Twin Development</li>
                  <li>Predictive Maintenance Systems</li>
                </ul>
              </div>

              <div className="bg-white rounded-2xl shadow-xl p-8 border border-gray-100 hover:shadow-2xl transition-shadow duration-300">
                <div className="flex items-center mb-6">
                  <div className="bg-yellow-100 p-4 rounded-xl">
                    <Lightbulb className="h-8 w-8 text-yellow-600" />
                  </div>
                  <h3 className="text-2xl font-bold ml-6">Nevadic Solar</h3>
                </div>
                <p className="text-gray-700 mb-6 text-lg">Nigerian solar products supplier with extensive experience in mini-grid ecosystem implementation and maintenance across Sub-Saharan Africa.</p>
                <ul className="list-disc pl-6 text-gray-700 space-y-2">
                  <li>Local Market Expertise</li>
                  <li>Distribution Network</li>
                  <li>Installation & Maintenance Support</li>
                </ul>
              </div>

              <div className="bg-white rounded-2xl shadow-xl p-8 border border-gray-100 hover:shadow-2xl transition-shadow duration-300">
                <div className="flex items-center mb-6">
                  <div className="bg-green-100 p-4 rounded-xl">
                    <Cpu className="h-8 w-8 text-green-600" />
                  </div>
                  <h3 className="text-2xl font-bold ml-6">AceOn Group</h3>
                </div>
                <p className="text-gray-700 mb-6 text-lg">Project lead with over 30 years of experience in battery technology and energy storage solutions, headquartered in Telford, UK.</p>
                <ul className="list-disc pl-6 text-gray-700 space-y-2">
                  <li>Custom Battery Pack Design</li>
                  <li>Energy Storage Systems</li>
                  <li>Global Distribution Network</li>
                </ul>
              </div>
            </div>

            <blockquote className="border-l-8 border-green-500 pl-8 my-16 py-4 bg-green-50 rounded-r-2xl">
              <p className="text-2xl italic text-gray-800 mb-4">
                "HIGHESS is not just a project; it's a commitment to empowering communities and bridging energy access gaps in regions where access to reliable electricity is very challenging."
              </p>
              <footer className="text-gray-600 text-xl">— Mark Thompson, CEO of AceOn Group</footer>
            </blockquote>

            <h2 className="text-4xl font-bold mb-12 text-gray-900">Project Impact</h2>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-16 mb-16">
              <div>
                <div className="flex items-center mb-8">
                  <Leaf className="h-8 w-8 text-green-600 mr-4" />
                  <h3 className="text-2xl font-bold">Environmental Impact</h3>
                </div>
                <ul className="space-y-6">
                  <li className="flex items-start">
                    <Target className="h-6 w-6 text-green-600 mr-4 mt-1 flex-shrink-0" />
                    <div>
                      <h4 className="text-lg font-semibold mb-2">Reduced Generator Dependency</h4>
                      <p className="text-gray-600">Significant reduction in diesel generator usage, leading to decreased carbon emissions and operational costs.</p>
                    </div>
                  </li>
                  <li className="flex items-start">
                    <Zap className="h-6 w-6 text-green-600 mr-4 mt-1 flex-shrink-0" />
                    <div>
                      <h4 className="text-lg font-semibold mb-2">Clean Energy Transition</h4>
                      <p className="text-gray-600">Implementation of sustainable energy solutions, promoting renewable power sources across communities.</p>
                    </div>
                  </li>
                  <li className="flex items-start">
                    <LineChart className="h-6 w-6 text-green-600 mr-4 mt-1 flex-shrink-0" />
                    <div>
                      <h4 className="text-lg font-semibold mb-2">Environmental Preservation</h4>
                      <p className="text-gray-600">Measurable reduction in carbon footprint through innovative energy storage solutions.</p>
                    </div>
                  </li>
                </ul>
              </div>

              <div>
                <div className="flex items-center mb-8">
                  <Users2 className="h-8 w-8 text-blue-600 mr-4" />
                  <h3 className="text-2xl font-bold">Social Impact</h3>
                </div>
                <ul className="space-y-6">
                  <li className="flex items-start">
                    <Award className="h-6 w-6 text-blue-600 mr-4 mt-1 flex-shrink-0" />
                    <div>
                      <h4 className="text-lg font-semibold mb-2">Community Empowerment</h4>
                      <p className="text-gray-600">Enabling local communities through reliable access to electricity and sustainable energy solutions.</p>
                    </div>
                  </li>
                  <li className="flex items-start">
                    <GanttChartSquare className="h-6 w-6 text-blue-600 mr-4 mt-1 flex-shrink-0" />
                    <div>
                      <h4 className="text-lg font-semibold mb-2">Economic Growth</h4>
                      <p className="text-gray-600">Creating employment opportunities and developing local technical expertise in renewable energy.</p>
                    </div>
                  </li>
                  <li className="flex items-start">
                    <Users className="h-6 w-6 text-blue-600 mr-4 mt-1 flex-shrink-0" />
                    <div>
                      <h4 className="text-lg font-semibold mb-2">Quality of Life</h4>
                      <p className="text-gray-600">Improving living standards through reliable electricity access for education, healthcare, and business.</p>
                    </div>
                  </li>
                </ul>
              </div>
            </div>

            <h2 className="text-4xl font-bold mb-12 text-gray-900">Funding Details</h2>
            <div className="mb-16">
              <div className="flex items-center mb-8">
                <Coins className="h-8 w-8 text-gray-700 mr-4" />
                <h3 className="text-2xl font-bold">Total Project Value: £1.4M</h3>
              </div>
              
              <div className="space-y-8">
                <div className="flex items-start">
                  <Building className="h-6 w-6 text-gray-700 mr-4 mt-1 flex-shrink-0" />
                  <div>
                    <h4 className="text-xl font-bold mb-2">Ayrton Fund</h4>
                    <p className="text-gray-600">Supporting clean energy innovation and sustainable development initiatives across emerging markets, with a focus on transformative technologies that address climate challenges.</p>
                  </div>
                </div>
                
                <div className="flex items-start">
                  <Landmark className="h-6 w-6 text-gray-700 mr-4 mt-1 flex-shrink-0" />
                  <div>
                    <h4 className="text-xl font-bold mb-2">Foreign, Commonwealth and Development Office (FCDO)</h4>
                    <p className="text-gray-600">Strategic support for international development projects that promote sustainable energy access and economic growth in developing regions.</p>
                  </div>
                </div>
                
                <div className="flex items-start">
                  <FlaskConical className="h-6 w-6 text-gray-700 mr-4 mt-1 flex-shrink-0" />
                  <div>
                    <h4 className="text-xl font-bold mb-2">Department for Science, Innovation and Technology (DSIT)</h4>
                    <p className="text-gray-600">Backing innovative technological solutions that advance the UK's leadership in sustainable energy and promote international collaboration in scientific research.</p>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Footer with Background */}
      <div className="relative">
        <div 
          className="absolute inset-0 bg-cover bg-center opacity-10"
          style={{
            backgroundImage: 'url("https://images.unsplash.com/photo-1509391366360-2e959784a276?auto=format&fit=crop&q=80")',
          }}
        />
        <footer className="relative bg-gradient-to-b from-gray-900 to-gray-950 text-white py-6">
          <div className="container mx-auto px-4 max-w-7xl">
            <div className="flex flex-wrap items-center justify-between">
              <div className="flex items-center space-x-8">
                <div>
                  <span className="text-lg font-bold">HIGHESS Project</span>
                  <span className="mx-3 text-gray-500">|</span>
                  <span className="text-gray-400">Transforming energy access</span>
                </div>
                <div className="flex items-center space-x-4">
                  <span className="text-gray-400">Contact:</span>
                  <a href="mailto:info@highess-project.com" className="hover:text-blue-400 transition-colors">info@highess-project.com</a>
                  <span className="text-gray-500">|</span>
                  <span>+44 (0) 1952 293 038</span>
                </div>
              </div>
              <div className="flex items-center space-x-6">
                <a 
                  href="/admin" 
                  className="inline-flex items-center bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded-lg transition-colors duration-200 shadow-lg"
                >
                  <Settings className="h-5 w-5 mr-2 animate-pulse" />
                  <span className="font-medium">BMS Portal</span>
                </a>
                <span className="text-sm text-gray-400">&copy; {new Date().getFullYear()} AceOn Group</span>
              </div>
            </div>
          </div>
        </footer>
      </div>
    </div>
  );
}

createRoot(document.getElementById('root')!).render(
    <StrictMode>
      <App />
    </StrictMode>
  );